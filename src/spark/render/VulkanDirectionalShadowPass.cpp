#include "spark/render/VulkanDirectionalShadowPass.hpp"

#include "spark/render/SceneLightingProfile.hpp"
#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/render/VulkanSceneMeshDraw.hpp"
#include "spark/render/VulkanSceneVertexLayout.hpp"
#include "spark/render/VulkanShadowCastDraw.hpp"
#include "spark/math/Matrix4.hpp"

#include <cstring>
#include <stdexcept>

namespace Spark {

bool VulkanDirectionalShadowPass::HasFlightDepthView(const std::uint32_t frameIndex) const noexcept {
    return frameIndex < flights_.GetSize() && flights_[frameIndex].depthView != VK_NULL_HANDLE;
}

void VulkanDirectionalShadowPass::DestroyGraphicsPipeline(const VkDevice device) {
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

void VulkanDirectionalShadowPass::DestroyResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t fi = 0; fi < flights_.GetSize(); ++fi) {
        FlightTarget& sh = flights_[fi];
        if (sh.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, sh.framebuffer, nullptr);
            sh.framebuffer = VK_NULL_HANDLE;
        }
        if (sh.depthView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, sh.depthView, nullptr);
            sh.depthView = VK_NULL_HANDLE;
        }
        if (sh.depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, sh.depthImage, nullptr);
            sh.depthImage = VK_NULL_HANDLE;
        }
        if (sh.depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, sh.depthMemory, nullptr);
            sh.depthMemory = VK_NULL_HANDLE;
        }
        sh.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

void VulkanDirectionalShadowPass::CreateResources(
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
    samplerInfo.mipLodBias = 0.0F;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0F;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0F;
    samplerInfo.maxLod = 1.0F;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &compareSampler_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSampler (shadow compare) failed");
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
    dep0.srcAccessMask = 0;
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
        throw std::runtime_error("vkCreateRenderPass (shadow) failed");
    }

    flights_.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < framesInFlight; ++fi) {
        FlightTarget& sh = flights_[fi];
        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                kMapSize,
                kMapSize,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                sh.depthImage,
                sh.depthMemory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = sh.depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &viewInfo, nullptr, &sh.depthView) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateImageView (shadow) failed");
        }
        sh.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass_;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &sh.depthView;
        fbInfo.width = kMapSize;
        fbInfo.height = kMapSize;
        fbInfo.layers = 1;
        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &sh.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer (shadow) failed");
        }
    }
}

void VulkanDirectionalShadowPass::CreateGraphicsPipeline(
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
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = 6;
    vertexInputInfo.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
    colorBlending.pAttachments = nullptr;

    Array<VkDynamicState> dynamicStatesList;
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStatesList.GetSize());
    dynamicState.pDynamicStates = dynamicStatesList.GetData();

    if (sceneDescriptorSetLayout == VK_NULL_HANDLE) {
        throw std::runtime_error("VulkanDirectionalShadowPass: descriptor set layout missing");
    }

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
        throw std::runtime_error("vkCreatePipelineLayout (shadow) failed");
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
        throw std::runtime_error("vkCreateGraphicsPipelines (shadow) failed");
    }
}

void VulkanDirectionalShadowPass::EnsureDepthImageReadable(
        const VkCommandBuffer commandBuffer,
        FlightTarget& flight) const {
    if (flight.depthImage == VK_NULL_HANDLE || flight.depthLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = flight.depthImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    flight.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

void VulkanDirectionalShadowPass::Record(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VulkanShadowRecordContext& ctx,
        const VulkanDirectionalShadowFrameState& frameState) {
    if (pipeline_ == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE || frameIndex >= flights_.GetSize()) {
        return;
    }

    FlightTarget& shadowFlight = flights_[frameIndex];
    EnsureDepthImageReadable(commandBuffer, shadowFlight);

    if (!frameState.active || flights_[frameIndex].framebuffer == VK_NULL_HANDLE ||
        flights_[frameIndex].depthImage == VK_NULL_HANDLE || !ctx.sceneParamsValid || ctx.vertexBuffer == VK_NULL_HANDLE ||
        ctx.indexBuffer == VK_NULL_HANDLE || ctx.descriptorSet == VK_NULL_HANDLE || ctx.scene == nullptr) {
        return;
    }

    const VkImageAspectFlags shadowAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = shadowFlight.depthLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = shadowFlight.depthImage;
    barrier.subresourceRange.aspectMask = shadowAspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccess = 0;
    if (shadowFlight.depthLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        srcAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    } else if (shadowFlight.depthLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

    VkClearValue clearDepth{};
    clearDepth.depthStencil.depth = 1.0F;
    clearDepth.depthStencil.stencil = 0;

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = renderPass_;
    rpBegin.framebuffer = shadowFlight.framebuffer;
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent.width = kMapSize;
    rpBegin.renderArea.extent.height = kMapSize;
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearDepth;

    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
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

    const float tileW = static_cast<float>(kCascadeTileSize);
    const float tileH = static_cast<float>(kCascadeTileSize);

    for (std::uint32_t cascade = 0; cascade < kCascadeCount; ++cascade) {
        VkViewport vp{};
        const std::uint32_t col = cascade % 2U;
        const std::uint32_t row = cascade / 2U;
        vp.x = static_cast<float>(col) * tileW;
        vp.y = static_cast<float>(row) * tileH;
        vp.width = tileW;
        vp.height = tileH;
        vp.minDepth = 0.0F;
        vp.maxDepth = 1.0F;
        vkCmdSetViewport(commandBuffer, 0, 1, &vp);

        VkRect2D sc{};
        sc.offset.x = static_cast<std::int32_t>(col * kCascadeTileSize);
        sc.offset.y = static_cast<std::int32_t>(row * kCascadeTileSize);
        sc.extent.width = kCascadeTileSize;
        sc.extent.height = kCascadeTileSize;
        vkCmdSetScissor(commandBuffer, 0, 1, &sc);

        RecordShadowCastMeshes(
                commandBuffer,
                pipeline_,
                pipelineLayout_,
                ctx,
                frameState.worldToShadowClip[cascade].m,
                frameIndex);
    }

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier sampleBarrier{};
    sampleBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    sampleBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    sampleBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    sampleBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    sampleBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    sampleBarrier.image = shadowFlight.depthImage;
    sampleBarrier.subresourceRange.aspectMask = shadowAspect;
    sampleBarrier.subresourceRange.baseMipLevel = 0;
    sampleBarrier.subresourceRange.levelCount = 1;
    sampleBarrier.subresourceRange.baseArrayLayer = 0;
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

    shadowFlight.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
}

}  // namespace Spark
