#include "spark/render/VulkanPunctualShadowPass.hpp"

#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/VulkanPunctualShadowGpu.hpp"
#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/render/VulkanSceneVertexLayout.hpp"
#include "spark/render/VulkanShadowCastDraw.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Spark {

namespace {

struct ShadowCandidate {
    std::size_t lightIndex = 0;
    float score = 0.0F;
};

float LightScore(const Vector3& camPos, const Vector3& lightPos, float intensity, const Vector3& color) {
    const Vector3 delta = lightPos - camPos;
    const float dist = delta.Length();
    const float lum = color.x * 0.2126F + color.y * 0.7152F + color.z * 0.0722F;
    return intensity * lum / (1.0F + dist * 0.08F);
}

Matrix4 BuildSpotShadowMatrix(const SceneSpotLight& spot) {
    Vector3 up{0.0F, 1.0F, 0.0F};
    if (std::fabs(Vector3::Dot(spot.directionWorld, up)) > 0.92F) {
        up = {1.0F, 0.0F, 0.0F};
    }
    const Vector3 target = spot.positionWorld + spot.directionWorld;
    const Matrix4 view = Matrix4::LookAt(spot.positionWorld, target, up);
    const float fovY = std::max(spot.outerConeRadians * 1.12F, 0.35F);
    const Matrix4 proj = Matrix4::PerspectiveVulkan(fovY, 1.0F, 0.08F, std::max(spot.range, 0.5F));
    return proj * view;
}

void BuildPointFaceMatrices(
        const Vector3& lightPos,
        const float range,
        Matrix4 outFaceViewProj[kPointShadowFaceCount]) {
    static const Vector3 kFaceDirs[kPointShadowFaceCount] = {
            {1.0F, 0.0F, 0.0F},
            {-1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            {0.0F, -1.0F, 0.0F},
            {0.0F, 0.0F, 1.0F},
            {0.0F, 0.0F, -1.0F},
    };
    static const Vector3 kFaceUps[kPointShadowFaceCount] = {
            {0.0F, -1.0F, 0.0F},
            {0.0F, -1.0F, 0.0F},
            {0.0F, 0.0F, -1.0F},
            {0.0F, 0.0F, 1.0F},
            {0.0F, -1.0F, 0.0F},
            {0.0F, -1.0F, 0.0F},
    };
    const float nearZ = 0.1F;
    const float farZ = std::max(range, 0.5F);
    const Matrix4 proj = Matrix4::PerspectiveVulkan(HalfPi, 1.0F, nearZ, farZ);
    for (std::uint32_t face = 0; face < kPointShadowFaceCount; ++face) {
        const Vector3 target = lightPos + kFaceDirs[face];
        const Matrix4 view = Matrix4::LookAt(lightPos, target, kFaceUps[face]);
        outFaceViewProj[face] = proj * view;
    }
}

void FillSpotAtlasUv(const std::uint32_t slot, float outAtlas[4]) {
    const std::uint32_t col = slot % 2U;
    const std::uint32_t row = slot / 2U;
    outAtlas[0] = static_cast<float>(col) * 0.5F;
    outAtlas[1] = static_cast<float>(row) * 0.5F;
    outAtlas[2] = 0.5F;
    outAtlas[3] = 0.5F;
}

}  // namespace

bool VulkanPunctualShadowPass::HasFlightResources(const std::uint32_t frameIndex) const noexcept {
    return frameIndex < flights_.GetSize() && flights_[frameIndex].spot.depthView != VK_NULL_HANDLE &&
           flights_[frameIndex].point.depthArrayView != VK_NULL_HANDLE;
}

VkBuffer VulkanPunctualShadowPass::SsboBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= flights_.GetSize()) {
        return VK_NULL_HANDLE;
    }
    return flights_[frameIndex].ssboBuffer;
}

void VulkanPunctualShadowPass::DestroyGraphicsPipeline(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (vertModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertModule_, nullptr);
        vertModule_ = VK_NULL_HANDLE;
    }
}

void VulkanPunctualShadowPass::DestroyResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t fi = 0; fi < flights_.GetSize(); ++fi) {
        FlightTarget& flight = flights_[fi];
        if (flight.ssboMapped != nullptr) {
            vkUnmapMemory(device, flight.ssboMemory);
            flight.ssboMapped = nullptr;
        }
        if (flight.ssboBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, flight.ssboBuffer, nullptr);
            flight.ssboBuffer = VK_NULL_HANDLE;
        }
        if (flight.ssboMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, flight.ssboMemory, nullptr);
            flight.ssboMemory = VK_NULL_HANDLE;
        }

        SpotFlightTarget& spot = flight.spot;
        if (spot.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, spot.framebuffer, nullptr);
            spot.framebuffer = VK_NULL_HANDLE;
        }
        if (spot.depthView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, spot.depthView, nullptr);
            spot.depthView = VK_NULL_HANDLE;
        }
        if (spot.depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, spot.depthImage, nullptr);
            spot.depthImage = VK_NULL_HANDLE;
        }
        if (spot.depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, spot.depthMemory, nullptr);
            spot.depthMemory = VK_NULL_HANDLE;
        }
        spot.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        PointFlightTarget& point = flight.point;
        for (std::size_t li = 0; li < point.layerFramebuffers.GetSize(); ++li) {
            if (point.layerFramebuffers[li] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(device, point.layerFramebuffers[li], nullptr);
            }
        }
        point.layerFramebuffers.Clear();
        for (std::size_t li = 0; li < point.layerViews.GetSize(); ++li) {
            if (point.layerViews[li] != VK_NULL_HANDLE) {
                vkDestroyImageView(device, point.layerViews[li], nullptr);
            }
        }
        point.layerViews.Clear();
        if (point.depthArrayView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, point.depthArrayView, nullptr);
            point.depthArrayView = VK_NULL_HANDLE;
        }
        if (point.depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, point.depthImage, nullptr);
            point.depthImage = VK_NULL_HANDLE;
        }
        if (point.depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, point.depthMemory, nullptr);
            point.depthMemory = VK_NULL_HANDLE;
        }
        point.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    flights_.Clear();

    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }
    if (compareSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, compareSampler_, nullptr);
        compareSampler_ = VK_NULL_HANDLE;
    }
}

void VulkanPunctualShadowPass::CreateResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const std::uint32_t framesInFlight) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        return;
    }
    DestroyResources(device);

    const VkFormat depthFormat = VulkanRendererGpu::FindDepthFormat(physicalDevice);
    const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &compareSampler_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSampler (punctual shadow) failed");
    }

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep0{};
    dep0.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep0.dstSubpass = 0;
    dep0.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dep0.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep0.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &depthAttachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &dep0;
    if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateRenderPass (punctual shadow) failed");
    }

    flights_.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < framesInFlight; ++fi) {
        FlightTarget& flight = flights_[fi];

        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kPunctualShadowGpuBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                flight.ssboBuffer,
                flight.ssboMemory);
        if (vkMapMemory(device, flight.ssboMemory, 0, kPunctualShadowGpuBytes, 0, &flight.ssboMapped) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory (punctual shadow SSBO) failed");
        }
        std::memset(flight.ssboMapped, 0, kPunctualShadowGpuBytes);

        SpotFlightTarget& spot = flight.spot;
        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                kSpotShadowAtlasSize,
                kSpotShadowAtlasSize,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                spot.depthImage,
                spot.depthMemory);

        VkImageViewCreateInfo spotViewInfo{};
        spotViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        spotViewInfo.image = spot.depthImage;
        spotViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        spotViewInfo.format = depthFormat;
        spotViewInfo.subresourceRange.aspectMask = aspectMask;
        spotViewInfo.subresourceRange.levelCount = 1;
        spotViewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &spotViewInfo, nullptr, &spot.depthView) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateImageView (spot shadow) failed");
        }

        VkFramebufferCreateInfo spotFbInfo{};
        spotFbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        spotFbInfo.renderPass = renderPass_;
        spotFbInfo.attachmentCount = 1;
        spotFbInfo.pAttachments = &spot.depthView;
        spotFbInfo.width = kSpotShadowAtlasSize;
        spotFbInfo.height = kSpotShadowAtlasSize;
        spotFbInfo.layers = 1;
        if (vkCreateFramebuffer(device, &spotFbInfo, nullptr, &spot.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer (spot shadow) failed");
        }
        spot.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        PointFlightTarget& point = flight.point;
        VulkanRendererGpu::CreateImage2DArray(
                physicalDevice,
                device,
                kPointShadowFaceSize,
                kPointShadowFaceSize,
                kPointShadowLayerCount,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                point.depthImage,
                point.depthMemory);

        point.depthArrayView = VulkanRendererGpu::CreateImageView2DArray(
                device,
                point.depthImage,
                depthFormat,
                kPointShadowLayerCount,
                VulkanRendererGpu::ImageAspectForFormat(depthFormat));

        point.layerViews.Resize(kPointShadowLayerCount);
        point.layerFramebuffers.Resize(kPointShadowLayerCount);
        for (std::uint32_t layer = 0; layer < kPointShadowLayerCount; ++layer) {
            VkImageViewCreateInfo layerViewInfo{};
            layerViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            layerViewInfo.image = point.depthImage;
            layerViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            layerViewInfo.format = depthFormat;
            layerViewInfo.subresourceRange.aspectMask = aspectMask;
            layerViewInfo.subresourceRange.levelCount = 1;
            layerViewInfo.subresourceRange.baseArrayLayer = layer;
            layerViewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device, &layerViewInfo, nullptr, &point.layerViews[layer]) != VK_SUCCESS) {
                throw std::runtime_error("vkCreateImageView (point shadow layer) failed");
            }

            VkFramebufferCreateInfo layerFbInfo{};
            layerFbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            layerFbInfo.renderPass = renderPass_;
            layerFbInfo.attachmentCount = 1;
            layerFbInfo.pAttachments = &point.layerViews[layer];
            layerFbInfo.width = kPointShadowFaceSize;
            layerFbInfo.height = kPointShadowFaceSize;
            layerFbInfo.layers = 1;
            if (vkCreateFramebuffer(device, &layerFbInfo, nullptr, &point.layerFramebuffers[layer]) != VK_SUCCESS) {
                throw std::runtime_error("vkCreateFramebuffer (point shadow layer) failed");
            }
        }
        point.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void VulkanPunctualShadowPass::CreateGraphicsPipeline(
        const VkDevice device,
        const VkDescriptorSetLayout sceneDescriptorSetLayout,
        const VulkanSpvShaderLoader& shaders) {
    static_assert(sizeof(VulkanShadowPushConstants) == 144);
    if (device == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    const Array<char> vertCode = shaders.ReadSpvFile("shadow_depth.vert.spv");
    vertModule_ = shaders.CreateShaderModule(vertCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule_;
    vertStage.pName = "main";

    using VL = VulkanSceneVertexLayout;
    constexpr std::uint32_t stride = VL::kStrideBytes;
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = stride;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[6]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = sizeof(float) * VL::kOffPosition;
    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = sizeof(float) * VL::kOffNormal;
    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = sizeof(float) * VL::kOffTexCoord;
    attrs[3].binding = 0;
    attrs[3].location = 3;
    attrs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[3].offset = sizeof(float) * VL::kOffTangent;
    attrs[4].binding = 0;
    attrs[4].location = 4;
    attrs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[4].offset = sizeof(float) * VL::kOffJoints;
    attrs[5].binding = 0;
    attrs[5].location = 5;
    attrs[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[5].offset = sizeof(float) * VL::kOffWeights;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 6;
    vertexInputInfo.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0F;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 4.75F;
    rasterizer.depthBiasSlopeFactor = 4.75F;
    rasterizer.depthBiasClamp = 0.0F;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 0;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(VulkanShadowPushConstants);

    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &sceneDescriptorSetLayout;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pcRange;
    if (vkCreatePipelineLayout(device, &plInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (punctual shadow) failed");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 1;
    pipelineCreateInfo.pStages = &vertStage;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisampling;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.layout = pipelineLayout_;
    pipelineCreateInfo.renderPass = renderPass_;
    pipelineCreateInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateGraphicsPipelines (punctual shadow) failed");
    }
}

void VulkanPunctualShadowPass::PrepareAndUpload(
        const std::uint32_t frameIndex,
        const SceneRenderParams& scene,
        const ResolvedSceneLighting& lighting,
        VulkanPunctualShadowFrameState& out) {
    out = VulkanPunctualShadowFrameState{};
    if (frameIndex >= flights_.GetSize() || flights_[frameIndex].ssboMapped == nullptr) {
        return;
    }

    PunctualShadowGpu gpu{};
    for (std::int32_t& slot : gpu.pointShadowSlotByLight) {
        slot = -1;
    }
    for (std::int32_t& slot : gpu.spotShadowSlotByLight) {
        slot = -1;
    }
    for (std::int32_t& idx : gpu.spotLightIndex) {
        idx = -1;
    }
    for (std::int32_t& idx : gpu.pointLightIndex) {
        idx = -1;
    }

    const bool enabled = lighting.punctualShadowsEnabled && scene.punctualShadowsEnabled && HasFlightResources(frameIndex);
    gpu.enabled = enabled ? 1U : 0U;
    if (!enabled) {
        std::memcpy(flights_[frameIndex].ssboMapped, &gpu, sizeof(gpu));
        return;
    }

    std::vector<ShadowCandidate> spotCands;
    spotCands.reserve(scene.spotLights.GetSize());
    for (std::size_t i = 0; i < scene.spotLights.GetSize(); ++i) {
        const SceneSpotLight& sl = scene.spotLights[i];
        if (!sl.castsShadow) {
            continue;
        }
        ShadowCandidate c{};
        c.lightIndex = i;
        c.score = LightScore(scene.cameraPositionWorld, sl.positionWorld, sl.intensity, sl.color);
        spotCands.push_back(c);
    }
    std::sort(spotCands.begin(), spotCands.end(), [](const ShadowCandidate& a, const ShadowCandidate& b) {
        return a.score > b.score;
    });

    std::vector<ShadowCandidate> pointCands;
    pointCands.reserve(scene.pointLights.GetSize());
    for (std::size_t i = 0; i < scene.pointLights.GetSize(); ++i) {
        const ScenePointLight& pl = scene.pointLights[i];
        if (!pl.castsShadow) {
            continue;
        }
        ShadowCandidate c{};
        c.lightIndex = i;
        c.score = LightScore(scene.cameraPositionWorld, pl.positionWorld, pl.intensity, pl.color);
        pointCands.push_back(c);
    }
    std::sort(pointCands.begin(), pointCands.end(), [](const ShadowCandidate& a, const ShadowCandidate& b) {
        return a.score > b.score;
    });

    const std::uint32_t numSpots =
            static_cast<std::uint32_t>(std::min(spotCands.size(), static_cast<std::size_t>(kMaxSpotShadowMaps)));
    const std::uint32_t numPoints =
            static_cast<std::uint32_t>(std::min(pointCands.size(), static_cast<std::size_t>(kMaxPointShadowMaps)));

    gpu.numSpotShadows = numSpots;
    gpu.numPointShadows = numPoints;
    out.numSpotShadows = numSpots;
    out.numPointShadows = numPoints;
    out.active = (numSpots > 0U || numPoints > 0U);

    for (std::uint32_t slot = 0; slot < numSpots; ++slot) {
        const SceneSpotLight& sl = scene.spotLights[spotCands[slot].lightIndex];
        gpu.spotLightIndex[slot] = static_cast<std::int32_t>(spotCands[slot].lightIndex);
        out.spotLightIndex[slot] = gpu.spotLightIndex[slot];
        gpu.spotShadowSlotByLight[spotCands[slot].lightIndex] = static_cast<std::int32_t>(slot);
        out.spotWorldToClip[slot] = BuildSpotShadowMatrix(sl);
        std::memcpy(gpu.spotWorldToClip[slot], out.spotWorldToClip[slot].m, sizeof(gpu.spotWorldToClip[slot]));
        FillSpotAtlasUv(slot, gpu.spotAtlas[slot]);
        std::memcpy(out.spotAtlas[slot], gpu.spotAtlas[slot], sizeof(out.spotAtlas[slot]));
    }

    for (std::uint32_t slot = 0; slot < numPoints; ++slot) {
        const ScenePointLight& pl = scene.pointLights[pointCands[slot].lightIndex];
        gpu.pointLightIndex[slot] = static_cast<std::int32_t>(pointCands[slot].lightIndex);
        out.pointLightIndex[slot] = gpu.pointLightIndex[slot];
        gpu.pointShadowSlotByLight[pointCands[slot].lightIndex] = static_cast<std::int32_t>(slot);
        gpu.pointBaseLayer[slot] = slot * kPointShadowFaceCount;
        out.pointBaseLayer[slot] = gpu.pointBaseLayer[slot];
        gpu.pointPosRange[slot][0] = pl.positionWorld.x;
        gpu.pointPosRange[slot][1] = pl.positionWorld.y;
        gpu.pointPosRange[slot][2] = pl.positionWorld.z;
        gpu.pointPosRange[slot][3] = pl.range;
        std::memcpy(out.pointPosRange[slot], gpu.pointPosRange[slot], sizeof(out.pointPosRange[slot]));
        BuildPointFaceMatrices(pl.positionWorld, pl.range, out.pointFaceViewProj[slot]);
        for (std::uint32_t face = 0; face < kPointShadowFaceCount; ++face) {
            std::memcpy(
                    gpu.pointFaceWorldToClip[slot][face],
                    out.pointFaceViewProj[slot][face].m,
                    sizeof(gpu.pointFaceWorldToClip[slot][face]));
        }
    }

    std::memcpy(flights_[frameIndex].ssboMapped, &gpu, sizeof(gpu));
}

void VulkanPunctualShadowPass::EnsureDepthImagesReadable(
        const VkCommandBuffer commandBuffer,
        FlightTarget& flight) const {
    if (flight.spot.depthImage != VK_NULL_HANDLE &&
        flight.spot.depthLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        TransitionDepthImage(
                commandBuffer,
                flight.spot.depthImage,
                flight.spot.depthLayout,
                1,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_READ_BIT);
    }
    if (flight.point.depthImage != VK_NULL_HANDLE &&
        flight.point.depthLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        TransitionDepthImage(
                commandBuffer,
                flight.point.depthImage,
                flight.point.depthLayout,
                kPointShadowLayerCount,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_READ_BIT);
    }
}

void VulkanPunctualShadowPass::TransitionDepthImage(
        const VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout& layoutInOut,
        const std::uint32_t layerCount,
        const VkImageLayout newLayout,
        const VkAccessFlags dstAccess) const {
    if (image == VK_NULL_HANDLE || layoutInOut == newLayout) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layoutInOut;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccess = 0;
    if (layoutInOut == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        srcAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    } else if (layoutInOut == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    if ((dstAccess & VK_ACCESS_SHADER_READ_BIT) != 0U) {
        dstStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if ((dstAccess & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0U) {
        dstStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    if ((dstAccess & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0U) {
        dstStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (dstStage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
        dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            dstStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    layoutInOut = newLayout;
}

void VulkanPunctualShadowPass::Record(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VulkanShadowRecordContext& ctx,
        const VulkanPunctualShadowFrameState& frameState) {
    if (pipeline_ == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE || frameIndex >= flights_.GetSize()) {
        return;
    }

    FlightTarget& flight = flights_[frameIndex];
    EnsureDepthImagesReadable(commandBuffer, flight);

    if (!frameState.active || !ctx.sceneParamsValid || ctx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }
    const VkClearValue clearDepth{.depthStencil = {1.0F, 0}};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout_,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    if (frameState.numSpotShadows > 0U && flight.spot.framebuffer != VK_NULL_HANDLE) {
        TransitionDepthImage(
                commandBuffer,
                flight.spot.depthImage,
                flight.spot.depthLayout,
                1,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = renderPass_;
        rpBegin.framebuffer = flight.spot.framebuffer;
        rpBegin.renderArea.offset = {0, 0};
        rpBegin.renderArea.extent = {kSpotShadowAtlasSize, kSpotShadowAtlasSize};
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clearDepth;
        vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        const float tileW = static_cast<float>(kSpotShadowTileSize);
        const float tileH = static_cast<float>(kSpotShadowTileSize);
        for (std::uint32_t slot = 0; slot < frameState.numSpotShadows; ++slot) {
            const std::uint32_t col = slot % 2U;
            const std::uint32_t row = slot / 2U;

            VkViewport vp{};
            vp.x = static_cast<float>(col) * tileW;
            vp.y = static_cast<float>(row) * tileH;
            vp.width = tileW;
            vp.height = tileH;
            vp.minDepth = 0.0F;
            vp.maxDepth = 1.0F;
            vkCmdSetViewport(commandBuffer, 0, 1, &vp);

            VkRect2D sc{};
            sc.offset.x = static_cast<std::int32_t>(col * kSpotShadowTileSize);
            sc.offset.y = static_cast<std::int32_t>(row * kSpotShadowTileSize);
            sc.extent = {kSpotShadowTileSize, kSpotShadowTileSize};
            vkCmdSetScissor(commandBuffer, 0, 1, &sc);

            RecordShadowCastMeshes(
                    commandBuffer,
                    pipeline_,
                    pipelineLayout_,
                    ctx,
                    frameState.spotWorldToClip[slot].m,
                    frameIndex);
        }

        vkCmdEndRenderPass(commandBuffer);
        flight.spot.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkImageMemoryBarrier sampleBarrier{};
        sampleBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sampleBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        sampleBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        sampleBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sampleBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sampleBarrier.image = flight.spot.depthImage;
        sampleBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        sampleBarrier.subresourceRange.levelCount = 1;
        sampleBarrier.subresourceRange.layerCount = 1;
        sampleBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sampleBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &sampleBarrier);
    }

    if (frameState.numPointShadows > 0U && flight.point.depthImage != VK_NULL_HANDLE) {
        TransitionDepthImage(
                commandBuffer,
                flight.point.depthImage,
                flight.point.depthLayout,
                kPointShadowLayerCount,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        for (std::uint32_t slot = 0; slot < frameState.numPointShadows; ++slot) {
            const std::uint32_t baseLayer = slot * kPointShadowFaceCount;
            for (std::uint32_t face = 0; face < kPointShadowFaceCount; ++face) {
                const std::uint32_t layer = baseLayer + face;
                if (layer >= flight.point.layerFramebuffers.GetSize()) {
                    continue;
                }

                VkRenderPassBeginInfo rpBegin{};
                rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rpBegin.renderPass = renderPass_;
                rpBegin.framebuffer = flight.point.layerFramebuffers[layer];
                rpBegin.renderArea.offset = {0, 0};
                rpBegin.renderArea.extent = {kPointShadowFaceSize, kPointShadowFaceSize};
                rpBegin.clearValueCount = 1;
                rpBegin.pClearValues = &clearDepth;
                vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport vp{};
                vp.x = 0.0F;
                vp.y = 0.0F;
                vp.width = static_cast<float>(kPointShadowFaceSize);
                vp.height = static_cast<float>(kPointShadowFaceSize);
                vp.minDepth = 0.0F;
                vp.maxDepth = 1.0F;
                vkCmdSetViewport(commandBuffer, 0, 1, &vp);

                VkRect2D sc{};
                sc.offset = {0, 0};
                sc.extent = {kPointShadowFaceSize, kPointShadowFaceSize};
                vkCmdSetScissor(commandBuffer, 0, 1, &sc);

                RecordShadowCastMeshes(
                        commandBuffer,
                        pipeline_,
                        pipelineLayout_,
                        ctx,
                        frameState.pointFaceViewProj[slot][face].m,
                        frameIndex);

                vkCmdEndRenderPass(commandBuffer);
            }
        }

        flight.point.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkImageMemoryBarrier sampleBarrier{};
        sampleBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sampleBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        sampleBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        sampleBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sampleBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sampleBarrier.image = flight.point.depthImage;
        sampleBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        sampleBarrier.subresourceRange.levelCount = 1;
        sampleBarrier.subresourceRange.layerCount = kPointShadowLayerCount;
        sampleBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sampleBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &sampleBarrier);
    }
}

}  // namespace Spark
