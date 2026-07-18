#include "spark/render/ui/VulkanScreenUiPass.hpp"

#include "spark/render/gpu/VulkanBlendAttachment.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/text/Font.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

void StableSortRectDrawIndices(const Array<ScreenRectDraw>& rects, Array<std::size_t>& indices) noexcept {
    indices.Clear();
    for (std::size_t i = 0; i < rects.GetSize(); ++i) {
        indices.PushBack(i);
    }
    const std::size_t n = indices.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        const std::size_t key = indices[i];
        const std::uint8_t keyBlend = GetSceneBlendModePassOrder(rects[key].blendMode);
        const std::uint32_t keyPaint = rects[key].paintOrder;
        std::size_t j = i;
        while (j > 0) {
            const std::size_t prev = indices[j - 1];
            const std::uint8_t prevBlend = GetSceneBlendModePassOrder(rects[prev].blendMode);
            if (prevBlend > keyBlend) {
                indices[j] = prev;
                --j;
                continue;
            }
            if (prevBlend < keyBlend) {
                break;
            }
            if (rects[prev].paintOrder > keyPaint) {
                indices[j] = prev;
                --j;
                continue;
            }
            break;
        }
        indices[j] = key;
    }
}

void StableSortTextDrawIndices(const Array<ScreenTextDraw>& texts, Array<std::size_t>& indices) noexcept {
    indices.Clear();
    for (std::size_t i = 0; i < texts.GetSize(); ++i) {
        indices.PushBack(i);
    }
    const std::size_t n = indices.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        const std::size_t key = indices[i];
        const std::uint8_t keyBlend = GetSceneBlendModePassOrder(texts[key].blendMode);
        const std::uint32_t keyPaint = texts[key].paintOrder;
        std::size_t j = i;
        while (j > 0) {
            const std::size_t prev = indices[j - 1];
            const std::uint8_t prevBlend = GetSceneBlendModePassOrder(texts[prev].blendMode);
            if (prevBlend > keyBlend) {
                indices[j] = prev;
                --j;
                continue;
            }
            if (prevBlend < keyBlend) {
                break;
            }
            if (texts[prev].paintOrder > keyPaint) {
                indices[j] = prev;
                --j;
                continue;
            }
            break;
        }
        indices[j] = key;
    }
}

}  // namespace

[[nodiscard]] VkPipeline PipelineFromBlendArray(
        const VkPipeline* pipelines,
        const SceneBlendMode mode) noexcept {
    const std::size_t index = static_cast<std::size_t>(mode);
    if (index < kSceneBlendModeCount && pipelines[index] != VK_NULL_HANDLE) {
        return pipelines[index];
    }
    return pipelines[static_cast<std::size_t>(kSceneBlendModeDefault)];
}

VkPipeline VulkanScreenUiPass::PipelineForSolidBlendMode(const SceneBlendMode mode) const noexcept {
    return PipelineFromBlendArray(solidPipelines, mode);
}

VkPipeline VulkanScreenUiPass::PipelineForTextBlendMode(const SceneBlendMode mode) const noexcept {
    return PipelineFromBlendArray(textPipelines, mode);
}

void VulkanScreenUiPass::CreateResources(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        std::uint32_t framesInFlight,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    DestroyResources(device);

    this->device = device;

    VkDescriptorSetLayoutBinding fontBinding{};
    fontBinding.binding = 0;
    fontBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontBinding.descriptorCount = 1;
    fontBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo textLayoutInfo{};
    textLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textLayoutInfo.bindingCount = 1;
    textLayoutInfo.pBindings = &fontBinding;
    if (vkCreateDescriptorSetLayout(device, &textLayoutInfo, nullptr, &textDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorSetLayout (text) failed");
    }

    VkDescriptorPoolSize textPoolSize{};
    textPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textPoolSize.descriptorCount = framesInFlight;

    VkDescriptorPoolCreateInfo textPoolInfo{};
    textPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    textPoolInfo.poolSizeCount = 1;
    textPoolInfo.pPoolSizes = &textPoolSize;
    textPoolInfo.maxSets = framesInFlight;
    if (vkCreateDescriptorPool(device, &textPoolInfo, nullptr, &textDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool (text) failed");
    }

    Array<VkDescriptorSetLayout> textLayouts;
    textLayouts.Resize(framesInFlight);
    for (std::size_t i = 0; i < framesInFlight; ++i) {
        textLayouts[i] = textDescriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo textAlloc{};
    textAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    textAlloc.descriptorPool = textDescriptorPool;
    textAlloc.descriptorSetCount = framesInFlight;
    textAlloc.pSetLayouts = textLayouts.GetData();
    textDescriptorSets.Resize(framesInFlight);
    if (vkAllocateDescriptorSets(device, &textAlloc, textDescriptorSets.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets (text) failed");
    }

    fontSampler = VulkanRendererGpu::CreateFontAtlasSampler(device);

    const Array<char> textVertSpv = shaders.ReadSpvFile("text.vert.spv");
    const Array<char> textFragSpv = shaders.ReadSpvFile("text.frag.spv");
    textVertModule = shaders.CreateShaderModule(textVertSpv);
    textFragModule = shaders.CreateShaderModule(textFragSpv);

    textFlightBuffers.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < framesInFlight; ++fi) {
        UiMeshFlightBuffers& mesh = textFlightBuffers[fi];
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kTextVertexBytes,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mesh.vertexBuffer,
                mesh.vertexMemory);
        if (vkMapMemory(device, mesh.vertexMemory, 0, kTextVertexBytes, 0, &mesh.vertexMapped) != VK_SUCCESS) {
            throw std::runtime_error("map text vertex buffer failed");
        }

        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kTextIndexBytes,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mesh.indexBuffer,
                mesh.indexMemory);
        if (vkMapMemory(device, mesh.indexMemory, 0, kTextIndexBytes, 0, &mesh.indexMapped) != VK_SUCCESS) {
            throw std::runtime_error("map text index buffer failed");
        }
    }

    const Array<char> solidVertSpv = shaders.ReadSpvFile("ui_solid.vert.spv");
    const Array<char> solidFragSpv = shaders.ReadSpvFile("ui_solid.frag.spv");
    solidVertModule = shaders.CreateShaderModule(solidVertSpv);
    solidFragModule = shaders.CreateShaderModule(solidFragSpv);

    solidFlightBuffers.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < framesInFlight; ++fi) {
        UiMeshFlightBuffers& mesh = solidFlightBuffers[fi];
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kSolidVertexBytes,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mesh.vertexBuffer,
                mesh.vertexMemory);
        if (vkMapMemory(device, mesh.vertexMemory, 0, kSolidVertexBytes, 0, &mesh.vertexMapped) != VK_SUCCESS) {
            throw std::runtime_error("map solid vertex buffer failed");
        }

        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kSolidIndexBytes,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mesh.indexBuffer,
                mesh.indexMemory);
        if (vkMapMemory(device, mesh.indexMemory, 0, kSolidIndexBytes, 0, &mesh.indexMapped) != VK_SUCCESS) {
            throw std::runtime_error("map solid index buffer failed");
        }
    }
}

void VulkanScreenUiPass::DestroyResources(VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    uploadedUiFont = nullptr;
    uploadedUiBoldFont = nullptr;
    for (std::size_t i = 0; i < retiredFontAtlases.GetSize(); ++i) {
        DestroyFontAtlas(retiredFontAtlases[i].atlas, device);
    }
    retiredFontAtlases.Clear();
    DestroyFontAtlas(activeFontAtlas, device);
    DestroyFontAtlas(pendingFontAtlas, device);
    if (fontStagingMapped != nullptr) {
        vkUnmapMemory(device, fontStagingMemory);
        fontStagingMapped = nullptr;
    }
    if (fontStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, fontStagingBuffer, nullptr);
        fontStagingBuffer = VK_NULL_HANDLE;
    }
    if (fontStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, fontStagingMemory, nullptr);
        fontStagingMemory = VK_NULL_HANDLE;
    }
    fontStagingCapacity = 0;
    fontUploadPending = false;
    this->device = VK_NULL_HANDLE;

    for (std::size_t fi = 0; fi < textFlightBuffers.GetSize(); ++fi) {
        UiMeshFlightBuffers& mesh = textFlightBuffers[fi];
        if (mesh.vertexMapped != nullptr) {
            vkUnmapMemory(device, mesh.vertexMemory);
            mesh.vertexMapped = nullptr;
        }
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
            mesh.vertexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, mesh.vertexMemory, nullptr);
            mesh.vertexMemory = VK_NULL_HANDLE;
        }
        if (mesh.indexMapped != nullptr) {
            vkUnmapMemory(device, mesh.indexMemory);
            mesh.indexMapped = nullptr;
        }
        if (mesh.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
            mesh.indexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, mesh.indexMemory, nullptr);
            mesh.indexMemory = VK_NULL_HANDLE;
        }
    }
    textFlightBuffers.Clear();

    textDescriptorSets.Clear();
    if (textDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, textDescriptorPool, nullptr);
        textDescriptorPool = VK_NULL_HANDLE;
    }
    if (textDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, textDescriptorSetLayout, nullptr);
        textDescriptorSetLayout = VK_NULL_HANDLE;
    }
    if (fontSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, fontSampler, nullptr);
        fontSampler = VK_NULL_HANDLE;
    }
    if (textVertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, textVertModule, nullptr);
        textVertModule = VK_NULL_HANDLE;
    }
    if (textFragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, textFragModule, nullptr);
        textFragModule = VK_NULL_HANDLE;
    }

    for (std::size_t fi = 0; fi < solidFlightBuffers.GetSize(); ++fi) {
        UiMeshFlightBuffers& mesh = solidFlightBuffers[fi];
        if (mesh.vertexMapped != nullptr) {
            vkUnmapMemory(device, mesh.vertexMemory);
            mesh.vertexMapped = nullptr;
        }
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
            mesh.vertexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, mesh.vertexMemory, nullptr);
            mesh.vertexMemory = VK_NULL_HANDLE;
        }
        if (mesh.indexMapped != nullptr) {
            vkUnmapMemory(device, mesh.indexMemory);
            mesh.indexMapped = nullptr;
        }
        if (mesh.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
            mesh.indexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, mesh.indexMemory, nullptr);
            mesh.indexMemory = VK_NULL_HANDLE;
        }
    }
    solidFlightBuffers.Clear();

    if (solidVertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, solidVertModule, nullptr);
        solidVertModule = VK_NULL_HANDLE;
    }
    if (solidFragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, solidFragModule, nullptr);
        solidFragModule = VK_NULL_HANDLE;
    }
}

void VulkanScreenUiPass::DestroyPipelines(VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t i = 0; i < kSceneBlendModeCount; ++i) {
        if (textPipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, textPipelines[i], nullptr);
            textPipelines[i] = VK_NULL_HANDLE;
        }
        if (solidPipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, solidPipelines[i], nullptr);
            solidPipelines[i] = VK_NULL_HANDLE;
        }
    }
    if (textPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, textPipelineLayout, nullptr);
        textPipelineLayout = VK_NULL_HANDLE;
    }
    if (solidPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, solidPipelineLayout, nullptr);
        solidPipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanScreenUiPass::CreatePipelines(VkDevice device, VkRenderPass presentRenderPass) {
    if (device == VK_NULL_HANDLE || presentRenderPass == VK_NULL_HANDLE || textVertModule == VK_NULL_HANDLE ||
        textFragModule == VK_NULL_HANDLE || textDescriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }

    DestroyPipelines(device);

    VkPipelineShaderStageCreateInfo textVertStage{};
    textVertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    textVertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    textVertStage.module = textVertModule;
    textVertStage.pName = "main";

    VkPipelineShaderStageCreateInfo textFragStage{};
    textFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    textFragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    textFragStage.module = textFragModule;
    textFragStage.pName = "main";

    const VkPipelineShaderStageCreateInfo textStages[] = {textVertStage, textFragStage};

    constexpr std::uint32_t kTextStride = sizeof(float) * 9;
    VkVertexInputBindingDescription textBinding{};
    textBinding.binding = 0;
    textBinding.stride = kTextStride;
    textBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription textAttrs[3]{};
    textAttrs[0].binding = 0;
    textAttrs[0].location = 0;
    textAttrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    textAttrs[0].offset = 0;
    textAttrs[1].binding = 0;
    textAttrs[1].location = 1;
    textAttrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    textAttrs[1].offset = sizeof(float) * 2;
    textAttrs[2].binding = 0;
    textAttrs[2].location = 2;
    textAttrs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    textAttrs[2].offset = sizeof(float) * 5;

    VkPipelineVertexInputStateCreateInfo textVtxIn{};
    textVtxIn.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    textVtxIn.vertexBindingDescriptionCount = 1;
    textVtxIn.pVertexBindingDescriptions = &textBinding;
    textVtxIn.vertexAttributeDescriptionCount = 3;
    textVtxIn.pVertexAttributeDescriptions = textAttrs;

    VkPipelineInputAssemblyStateCreateInfo textIa{};
    textIa.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    textIa.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo textVp{};
    textVp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    textVp.viewportCount = 1;
    textVp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo textRast{};
    textRast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    textRast.polygonMode = VK_POLYGON_MODE_FILL;
    textRast.lineWidth = 1.0F;
    textRast.cullMode = VK_CULL_MODE_NONE;
    textRast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo textMs{};
    textMs.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    textMs.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo textDs{};
    textDs.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    textDs.depthTestEnable = VK_FALSE;
    textDs.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState textBlendAtt{};
    textBlendAtt.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    textBlendAtt.blendEnable = VK_TRUE;
    textBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    textBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    textBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    textBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    textBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    textBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo textBlend{};
    textBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    textBlend.attachmentCount = 1;
    textBlend.pAttachments = &textBlendAtt;

    const VkDynamicState textDynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo textDyn{};
    textDyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    textDyn.dynamicStateCount = 2u;
    textDyn.pDynamicStates = textDynStates;

    VkPushConstantRange textPc{};
    textPc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    textPc.offset = 0;
    textPc.size = sizeof(TextPushConstants);

    VkPipelineLayoutCreateInfo textPl{};
    textPl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    textPl.setLayoutCount = 1;
    textPl.pSetLayouts = &textDescriptorSetLayout;
    textPl.pushConstantRangeCount = 1;
    textPl.pPushConstantRanges = &textPc;
    if (vkCreatePipelineLayout(device, &textPl, nullptr, &textPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (text) failed");
    }

    VkGraphicsPipelineCreateInfo textPipe{};
    textPipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    textPipe.stageCount = 2;
    textPipe.pStages = textStages;
    textPipe.pVertexInputState = &textVtxIn;
    textPipe.pInputAssemblyState = &textIa;
    textPipe.pViewportState = &textVp;
    textPipe.pRasterizationState = &textRast;
    textPipe.pMultisampleState = &textMs;
    textPipe.pDepthStencilState = &textDs;
    textPipe.pColorBlendState = &textBlend;
    textPipe.pDynamicState = &textDyn;
    textPipe.layout = textPipelineLayout;
    textPipe.renderPass = presentRenderPass;
    textPipe.subpass = 0;

    for (std::size_t mi = 0; mi < kSceneBlendModeCount; ++mi) {
        const auto mode = static_cast<SceneBlendMode>(mi);
        if (VulkanCreateGraphicsPipelineForBlendMode(device, textPipe, mode, &textPipelines[mi]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipelines (text blend) failed");
        }
    }

    if (solidVertModule == VK_NULL_HANDLE || solidFragModule == VK_NULL_HANDLE) {
        return;
    }

    VkPipelineShaderStageCreateInfo solidVertStage{};
    solidVertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    solidVertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    solidVertStage.module = solidVertModule;
    solidVertStage.pName = "main";

    VkPipelineShaderStageCreateInfo solidFragStage{};
    solidFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    solidFragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    solidFragStage.module = solidFragModule;
    solidFragStage.pName = "main";

    const VkPipelineShaderStageCreateInfo solidStages[] = {solidVertStage, solidFragStage};

    constexpr std::uint32_t kSolidStride = sizeof(float) * 6;
    VkVertexInputBindingDescription solidBinding{};
    solidBinding.binding = 0;
    solidBinding.stride = kSolidStride;
    solidBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription solidAttrs[2]{};
    solidAttrs[0].binding = 0;
    solidAttrs[0].location = 0;
    solidAttrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    solidAttrs[0].offset = 0;
    solidAttrs[1].binding = 0;
    solidAttrs[1].location = 1;
    solidAttrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    solidAttrs[1].offset = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo solidVtxIn{};
    solidVtxIn.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    solidVtxIn.vertexBindingDescriptionCount = 1;
    solidVtxIn.pVertexBindingDescriptions = &solidBinding;
    solidVtxIn.vertexAttributeDescriptionCount = 2;
    solidVtxIn.pVertexAttributeDescriptions = solidAttrs;

    VkPipelineInputAssemblyStateCreateInfo solidIa{};
    solidIa.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    solidIa.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo solidVp{};
    solidVp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    solidVp.viewportCount = 1;
    solidVp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo solidRast{};
    solidRast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    solidRast.polygonMode = VK_POLYGON_MODE_FILL;
    solidRast.lineWidth = 1.0F;
    solidRast.cullMode = VK_CULL_MODE_NONE;
    solidRast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo solidMs{};
    solidMs.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    solidMs.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo solidDs{};
    solidDs.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    solidDs.depthTestEnable = VK_FALSE;
    solidDs.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState solidBlendAtt{};
    solidBlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
    solidBlendAtt.blendEnable = VK_TRUE;
    solidBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    solidBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    solidBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    solidBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    solidBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    solidBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
    solidBlendAtt.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo solidBlend{};
    solidBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    solidBlend.attachmentCount = 1;
    solidBlend.pAttachments = &solidBlendAtt;

    const VkDynamicState solidDynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo solidDyn{};
    solidDyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    solidDyn.dynamicStateCount = 2u;
    solidDyn.pDynamicStates = solidDynStates;

    VkPushConstantRange solidPc{};
    solidPc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    solidPc.offset = 0;
    solidPc.size = sizeof(TextPushConstants);

    VkPipelineLayoutCreateInfo solidPl{};
    solidPl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    solidPl.setLayoutCount = 0;
    solidPl.pSetLayouts = nullptr;
    solidPl.pushConstantRangeCount = 1;
    solidPl.pPushConstantRanges = &solidPc;
    if (vkCreatePipelineLayout(device, &solidPl, nullptr, &solidPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (solid UI) failed");
    }

    VkGraphicsPipelineCreateInfo solidPipe{};
    solidPipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    solidPipe.stageCount = 2;
    solidPipe.pStages = solidStages;
    solidPipe.pVertexInputState = &solidVtxIn;
    solidPipe.pInputAssemblyState = &solidIa;
    solidPipe.pViewportState = &solidVp;
    solidPipe.pRasterizationState = &solidRast;
    solidPipe.pMultisampleState = &solidMs;
    solidPipe.pDepthStencilState = &solidDs;
    solidPipe.pColorBlendState = &solidBlend;
    solidPipe.pDynamicState = &solidDyn;
    solidPipe.layout = solidPipelineLayout;
    solidPipe.renderPass = presentRenderPass;
    solidPipe.subpass = 0;

    for (std::size_t mi = 0; mi < kSceneBlendModeCount; ++mi) {
        const auto mode = static_cast<SceneBlendMode>(mi);
        if (VulkanCreateGraphicsPipelineForBlendMode(device, solidPipe, mode, &solidPipelines[mi]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipelines (solid UI blend) failed");
        }
    }
}

void VulkanScreenUiPass::AppendUiSolidRectGeometry(const ScreenRectDraw& rd) {
    auto pushV = [this](const float px, const float py, const float r, const float g, const float b, const float a) {
        solidScratchVertices.PushBack(px);
        solidScratchVertices.PushBack(py);
        solidScratchVertices.PushBack(r);
        solidScratchVertices.PushBack(g);
        solidScratchVertices.PushBack(b);
        solidScratchVertices.PushBack(a);
    };
    const float x0 = rd.x;
    const float y0 = rd.y;
    const float x1 = rd.x + rd.width;
    const float y1 = rd.y + rd.height;
    const float a = rd.alpha;
    const float r0 = rd.color.x;
    const float g0 = rd.color.y;
    const float b0 = rd.color.z;
    const float r1 = rd.colorB.x;
    const float g1 = rd.colorB.y;
    const float b1 = rd.colorB.z;
    const std::uint32_t base = static_cast<std::uint32_t>(solidScratchVertices.GetSize() / 6U);
    if (rd.gradient == ScreenRectGradient::Vertical) {
        pushV(x0, y0, r0, g0, b0, a);
        pushV(x1, y0, r0, g0, b0, a);
        pushV(x1, y1, r1, g1, b1, a);
        pushV(x0, y1, r1, g1, b1, a);
    } else if (rd.gradient == ScreenRectGradient::Horizontal) {
        pushV(x0, y0, r0, g0, b0, a);
        pushV(x1, y0, r1, g1, b1, a);
        pushV(x1, y1, r1, g1, b1, a);
        pushV(x0, y1, r0, g0, b0, a);
    } else {
        pushV(x0, y0, r0, g0, b0, a);
        pushV(x1, y0, r0, g0, b0, a);
        pushV(x1, y1, r0, g0, b0, a);
        pushV(x0, y1, r0, g0, b0, a);
    }
    solidScratchIndices.PushBack(base);
    solidScratchIndices.PushBack(base + 1);
    solidScratchIndices.PushBack(base + 2);
    solidScratchIndices.PushBack(base);
    solidScratchIndices.PushBack(base + 2);
    solidScratchIndices.PushBack(base + 3);
}

void VulkanScreenUiPass::FlushAccumulatedUiSolids(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const ScreenRectDraw& clipRef,
        VkPipeline pipeline) {
    if (pipeline == VK_NULL_HANDLE || solidScratchIndices.IsEmpty() || frameIndex >= solidFlightBuffers.GetSize()) {
        return;
    }
    UiMeshFlightBuffers& mesh = solidFlightBuffers[frameIndex];
    const VkDeviceSize vbBytes = sizeof(float) * static_cast<VkDeviceSize>(solidScratchVertices.GetSize());
    const VkDeviceSize ibBytes = sizeof(std::uint32_t) * static_cast<VkDeviceSize>(solidScratchIndices.GetSize());
    if (vbBytes > kSolidVertexBytes || ibBytes > kSolidIndexBytes || mesh.vertexMapped == nullptr ||
        mesh.indexMapped == nullptr) {
        solidScratchVertices.Clear();
        solidScratchIndices.Clear();
        return;
    }
    std::memcpy(mesh.vertexMapped, solidScratchVertices.GetData(), static_cast<std::size_t>(vbBytes));
    std::memcpy(mesh.indexMapped, solidScratchIndices.GetData(), static_cast<std::size_t>(ibBytes));

    VkViewport solidVp{};
    solidVp.x = 0.0F;
    solidVp.y = 0.0F;
    solidVp.width = static_cast<float>(extent.width);
    solidVp.height = static_cast<float>(extent.height);
    solidVp.minDepth = 0.0F;
    solidVp.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &solidVp);

    VkRect2D solidSc{};
    if (!VulkanScreenUiClip::ScreenRectToVkScissor(
                clipRef.clipEnabled,
                clipRef.clipX,
                clipRef.clipY,
                clipRef.clipW,
                clipRef.clipH,
                extent,
                solidSc)) {
        solidScratchVertices.Clear();
        solidScratchIndices.Clear();
        return;
    }
    vkCmdSetScissor(commandBuffer, 0, 1, &solidSc);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.vertexBuffer, &vbOff);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    TextPushConstants scp{};
    scp.screenSize[0] = static_cast<float>(extent.width);
    scp.screenSize[1] = static_cast<float>(extent.height);
    scp.screenSize[2] = 0.0F;
    scp.screenSize[3] = 0.0F;
    vkCmdPushConstants(
            commandBuffer,
            solidPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(TextPushConstants),
            &scp);

    vkCmdDrawIndexed(
            commandBuffer,
            static_cast<std::uint32_t>(solidScratchIndices.GetSize()),
            1,
            0,
            0,
            0);

    solidScratchVertices.Clear();
    solidScratchIndices.Clear();
}

void VulkanScreenUiPass::RecordSolidRectsForPipeline(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const Array<ScreenRectDraw>& rects) {
    if (PipelineForSolidBlendMode(kSceneBlendModeDefault) == VK_NULL_HANDLE || rects.IsEmpty()) {
        return;
    }

    solidScratchVertices.Clear();
    solidScratchIndices.Clear();

    Array<std::size_t> drawOrder;
    StableSortRectDrawIndices(rects, drawOrder);

    std::size_t batchClipRectIndex = 0;
    bool haveSolidBatch = false;

    constexpr VkDeviceSize kBytesPerSolidRectVerts = sizeof(float) * 4U * 6U;
    constexpr VkDeviceSize kBytesPerSolidRectIndices = sizeof(std::uint32_t) * 6U;

    for (std::size_t oi = 0; oi < drawOrder.GetSize(); ++oi) {
        const std::size_t ri = drawOrder[oi];
        const ScreenRectDraw& rd = rects[ri];
        if (rd.width <= 0.0F || rd.height <= 0.0F) {
            continue;
        }
        if (haveSolidBatch && !VulkanScreenUiClip::SameSolidRectBatch(rects[batchClipRectIndex], rd)) {
            FlushAccumulatedUiSolids(
                    commandBuffer,
                    frameIndex,
                    extent,
                    rects[batchClipRectIndex],
                    PipelineForSolidBlendMode(rects[batchClipRectIndex].blendMode));
            haveSolidBatch = false;
        }
        if (!haveSolidBatch) {
            batchClipRectIndex = ri;
            haveSolidBatch = true;
        }
        if (haveSolidBatch) {
            const VkDeviceSize nextVb =
                    sizeof(float) * static_cast<VkDeviceSize>(solidScratchVertices.GetSize()) + kBytesPerSolidRectVerts;
            const VkDeviceSize nextIb =
                    sizeof(std::uint32_t) * static_cast<VkDeviceSize>(solidScratchIndices.GetSize()) +
                    kBytesPerSolidRectIndices;
            if (nextVb > kSolidVertexBytes || nextIb > kSolidIndexBytes) {
                FlushAccumulatedUiSolids(
                        commandBuffer,
                        frameIndex,
                        extent,
                        rects[batchClipRectIndex],
                        PipelineForSolidBlendMode(rects[batchClipRectIndex].blendMode));
                batchClipRectIndex = ri;
                haveSolidBatch = true;
            }
        }
        AppendUiSolidRectGeometry(rd);
    }
    if (haveSolidBatch) {
        FlushAccumulatedUiSolids(
                commandBuffer,
                frameIndex,
                extent,
                rects[batchClipRectIndex],
                PipelineForSolidBlendMode(rects[batchClipRectIndex].blendMode));
    }
}

void VulkanScreenUiPass::RecordSolidRectsFor(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const Array<ScreenRectDraw>& rects) {
    if (PipelineForSolidBlendMode(kSceneBlendModeDefault) == VK_NULL_HANDLE || rects.IsEmpty()) {
        return;
    }

    RecordSolidRectsForPipeline(commandBuffer, frameIndex, extent, rects);
}

void VulkanScreenUiPass::UpdateTextDescriptorImage(VkDevice device) {
    if (device == VK_NULL_HANDLE || activeFontAtlas.view == VK_NULL_HANDLE || fontSampler == VK_NULL_HANDLE ||
        textDescriptorSets.IsEmpty()) {
        return;
    }
    VkDescriptorImageInfo img{};
    img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img.imageView = activeFontAtlas.view;
    img.sampler = fontSampler;
    for (std::size_t i = 0; i < textDescriptorSets.GetSize(); ++i) {
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = textDescriptorSets[i];
        w.dstBinding = 0;
        w.dstArrayElement = 0;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount = 1;
        w.pImageInfo = &img;
        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }
}

void VulkanScreenUiPass::DestroyFontAtlas(FontAtlasGpu& atlas, const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (atlas.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, atlas.view, nullptr);
        atlas.view = VK_NULL_HANDLE;
    }
    if (atlas.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, atlas.image, nullptr);
        atlas.image = VK_NULL_HANDLE;
    }
    if (atlas.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, atlas.memory, nullptr);
        atlas.memory = VK_NULL_HANDLE;
    }
    atlas.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    atlas.width = 0;
    atlas.height = 0;
}

void VulkanScreenUiPass::QueueRetireFontAtlas(FontAtlasGpu&& atlas, const std::uint64_t safeAfterFrame) {
    if (atlas.image == VK_NULL_HANDLE && atlas.view == VK_NULL_HANDLE) {
        return;
    }
    RetiredFontAtlas entry{};
    entry.atlas = atlas;
    entry.safeAfterFrame = safeAfterFrame;
    retiredFontAtlases.PushBack(entry);
    atlas = {};
}

void VulkanScreenUiPass::EnsureFontStagingCapacity(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkDeviceSize bytes) {
    if (bytes <= fontStagingCapacity) {
        return;
    }
    if (fontStagingMapped != nullptr) {
        vkUnmapMemory(device, fontStagingMemory);
        fontStagingMapped = nullptr;
    }
    if (fontStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, fontStagingBuffer, nullptr);
        fontStagingBuffer = VK_NULL_HANDLE;
    }
    if (fontStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, fontStagingMemory, nullptr);
        fontStagingMemory = VK_NULL_HANDLE;
    }
    fontStagingCapacity = bytes;
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            fontStagingCapacity,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            fontStagingBuffer,
            fontStagingMemory);
    if (vkMapMemory(device, fontStagingMemory, 0, fontStagingCapacity, 0, &fontStagingMapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanScreenUiPass: font staging map failed");
    }
}

bool VulkanScreenUiPass::NeedsFontUpload(const SceneRenderParams& scene) const noexcept {
    const Font* wantReg = scene.uiFont.Get();
    const Font* wantBold = scene.uiBoldFont.Get();
    return wantReg != uploadedUiFont || wantBold != uploadedUiBoldFont;
}

void VulkanScreenUiPass::PrepareFontUpload(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const SceneRenderParams& scene,
        const std::uint64_t frameCounter,
        const std::uint32_t maxFramesInFlight) {
    fontUploadPending = false;
    if (device == VK_NULL_HANDLE || !NeedsFontUpload(scene)) {
        return;
    }

    pendingUiFont = scene.uiFont.Get();
    pendingUiBoldFont = scene.uiBoldFont.Get();
    fontRetireAfterFrame = frameCounter + static_cast<std::uint64_t>(maxFramesInFlight);

    DestroyFontAtlas(pendingFontAtlas, device);

    if (pendingUiFont == nullptr || !pendingUiFont->IsValid()) {
        fontUploadClearsAtlas = true;
        fontUploadPending = true;
        return;
    }

    const Texture2D& regAtlas = pendingUiFont->GetAtlas();
    const std::uint32_t w = regAtlas.GetWidth();
    const std::uint32_t h = regAtlas.GetHeight();
    if (w == 0 || h == 0) {
        return;
    }

    const Array<std::uint8_t>& regPx = regAtlas.GetRgba();
    pendingFontLayerBytes = static_cast<VkDeviceSize>(regPx.GetSize());
    const VkDeviceSize stagingBytes = pendingFontLayerBytes * 2U;
    EnsureFontStagingCapacity(physicalDevice, device, stagingBytes);

    const Font* boldForLayer1 = nullptr;
    if (pendingUiBoldFont != nullptr && pendingUiBoldFont->IsValid()) {
        const Texture2D& bAt = pendingUiBoldFont->GetAtlas();
        if (bAt.GetWidth() == w && bAt.GetHeight() == h && bAt.GetRgba().GetSize() == regPx.GetSize()) {
            boldForLayer1 = pendingUiBoldFont;
        }
    }

    auto* bytes = static_cast<std::uint8_t*>(fontStagingMapped);
    std::memcpy(bytes, regPx.GetData(), static_cast<std::size_t>(pendingFontLayerBytes));
    if (boldForLayer1 != nullptr) {
        std::memcpy(
                bytes + static_cast<std::size_t>(pendingFontLayerBytes),
                boldForLayer1->GetAtlas().GetRgba().GetData(),
                static_cast<std::size_t>(pendingFontLayerBytes));
    } else {
        std::memcpy(
                bytes + static_cast<std::size_t>(pendingFontLayerBytes),
                regPx.GetData(),
                static_cast<std::size_t>(pendingFontLayerBytes));
    }

    VulkanRendererGpu::CreateImage2DArray(
            physicalDevice,
            device,
            w,
            h,
            2,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            pendingFontAtlas.image,
            pendingFontAtlas.memory);
    pendingFontAtlas.view =
            VulkanRendererGpu::CreateImageView2DArray(device, pendingFontAtlas.image, VK_FORMAT_R8G8B8A8_UNORM, 2);
    pendingFontAtlas.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    pendingFontAtlas.width = w;
    pendingFontAtlas.height = h;

    fontUploadClearsAtlas = false;
    fontUploadPending = true;
}

void VulkanScreenUiPass::RecordFontUpload(const VkCommandBuffer commandBuffer, const VkDevice device) {
    if (!fontUploadPending || commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    if (fontUploadClearsAtlas) {
        QueueRetireFontAtlas(MoveTemp(activeFontAtlas), fontRetireAfterFrame);
        uploadedUiFont = nullptr;
        uploadedUiBoldFont = nullptr;
        fontUploadPending = false;
        fontUploadClearsAtlas = false;
        UpdateTextDescriptorImage(device);
        return;
    }

    if (pendingFontAtlas.image == VK_NULL_HANDLE || fontStagingBuffer == VK_NULL_HANDLE ||
        pendingFontLayerBytes == 0) {
        fontUploadPending = false;
        return;
    }

    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer,
            pendingFontAtlas.image,
            2,
            pendingFontAtlas.layout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy regions[2]{};
    for (int i = 0; i < 2; ++i) {
        regions[static_cast<std::size_t>(i)].bufferOffset = static_cast<VkDeviceSize>(i) * pendingFontLayerBytes;
        regions[static_cast<std::size_t>(i)].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[static_cast<std::size_t>(i)].imageSubresource.mipLevel = 0;
        regions[static_cast<std::size_t>(i)].imageSubresource.baseArrayLayer = static_cast<std::uint32_t>(i);
        regions[static_cast<std::size_t>(i)].imageSubresource.layerCount = 1;
        regions[static_cast<std::size_t>(i)].imageExtent = {pendingFontAtlas.width, pendingFontAtlas.height, 1};
    }
    vkCmdCopyBufferToImage(
            commandBuffer,
            fontStagingBuffer,
            pendingFontAtlas.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            2,
            regions);
    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer,
            pendingFontAtlas.image,
            2,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pendingFontAtlas.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    QueueRetireFontAtlas(MoveTemp(activeFontAtlas), fontRetireAfterFrame);
    activeFontAtlas = pendingFontAtlas;
    pendingFontAtlas = {};
    uploadedUiFont = pendingUiFont;
    uploadedUiBoldFont = pendingUiBoldFont;
    fontUploadPending = false;
    UpdateTextDescriptorImage(device);
}

void VulkanScreenUiPass::ReleaseRetiredFontAtlases(const VkDevice device, const std::uint64_t frameCounter) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    std::size_t write = 0;
    for (std::size_t i = 0; i < retiredFontAtlases.GetSize(); ++i) {
        if (retiredFontAtlases[i].safeAfterFrame <= frameCounter) {
            DestroyFontAtlas(retiredFontAtlases[i].atlas, device);
        } else {
            if (write != i) {
                retiredFontAtlases[write] = retiredFontAtlases[i];
            }
            ++write;
        }
    }
    retiredFontAtlases.Resize(write);
}

void VulkanScreenUiPass::AppendUiTextDrawGeometry(
        const ScreenTextDraw& td, const Font* regFont, const Font* boldFont, bool boldAtlasOk) {
    auto appendGeo = [&](const Font* f, const float ox, const float atlasLayer) {
        if (f == nullptr || !f->IsValid()) {
            return;
        }
        f->AppendTextGeometry(
                td.text,
                td.x + ox,
                td.y,
                td.sizePixels,
                td.color,
                td.alpha,
                atlasLayer,
                textScratchVertices,
                textScratchIndices);
    };
    if (td.bold && boldAtlasOk) {
        appendGeo(boldFont, 0.0F, 1.0F);
    } else if (td.bold) {
        const float e = std::max(0.75F, td.sizePixels * 0.016F);
        appendGeo(regFont, -e, 0.0F);
        appendGeo(regFont, 0.0F, 0.0F);
        appendGeo(regFont, e, 0.0F);
    } else {
        appendGeo(regFont, 0.0F, 0.0F);
    }
}

void VulkanScreenUiPass::FlushAccumulatedUiTexts(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const ScreenTextDraw& clipRef,
        VkPipeline pipeline) {
    if (pipeline == VK_NULL_HANDLE || textScratchIndices.IsEmpty() || frameIndex >= textFlightBuffers.GetSize()) {
        return;
    }
    UiMeshFlightBuffers& mesh = textFlightBuffers[frameIndex];
    const VkDeviceSize vbBytes = sizeof(float) * static_cast<VkDeviceSize>(textScratchVertices.GetSize());
    const VkDeviceSize ibBytes = sizeof(std::uint32_t) * static_cast<VkDeviceSize>(textScratchIndices.GetSize());
    if (vbBytes > kTextVertexBytes || ibBytes > kTextIndexBytes || mesh.vertexMapped == nullptr ||
        mesh.indexMapped == nullptr) {
        textScratchVertices.Clear();
        textScratchIndices.Clear();
        return;
    }
    std::memcpy(mesh.vertexMapped, textScratchVertices.GetData(), static_cast<std::size_t>(vbBytes));
    std::memcpy(mesh.indexMapped, textScratchIndices.GetData(), static_cast<std::size_t>(ibBytes));

    VkViewport textViewport{};
    textViewport.x = 0.0F;
    textViewport.y = 0.0F;
    textViewport.width = static_cast<float>(extent.width);
    textViewport.height = static_cast<float>(extent.height);
    textViewport.minDepth = 0.0F;
    textViewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &textViewport);

    VkRect2D textScissor{};
    if (!VulkanScreenUiClip::ScreenRectToVkScissor(
                clipRef.clipEnabled,
                clipRef.clipX,
                clipRef.clipY,
                clipRef.clipW,
                clipRef.clipH,
                extent,
                textScissor)) {
        textScratchVertices.Clear();
        textScratchIndices.Clear();
        return;
    }
    vkCmdSetScissor(commandBuffer, 0, 1, &textScissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.vertexBuffer, &vbOff);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            textPipelineLayout,
            0,
            1,
            &textDescriptorSets[frameIndex],
            0,
            nullptr);

    TextPushConstants tcp{};
    tcp.screenSize[0] = static_cast<float>(extent.width);
    tcp.screenSize[1] = static_cast<float>(extent.height);
    tcp.screenSize[2] = 0.0F;
    tcp.screenSize[3] = 0.0F;
    vkCmdPushConstants(
            commandBuffer,
            textPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(TextPushConstants),
            &tcp);

    vkCmdDrawIndexed(
            commandBuffer,
            static_cast<std::uint32_t>(textScratchIndices.GetSize()),
            1,
            0,
            0,
            0);

    textScratchVertices.Clear();
    textScratchIndices.Clear();
}

void VulkanScreenUiPass::RecordTextOverlaysFor(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const SceneRenderParams& scene,
        const Array<ScreenTextDraw>& texts) {
    if (PipelineForTextBlendMode(kSceneBlendModeDefault) == VK_NULL_HANDLE || uploadedUiFont == nullptr ||
        activeFontAtlas.view == VK_NULL_HANDLE || texts.IsEmpty() ||
        frameIndex >= textDescriptorSets.GetSize()) {
        return;
    }
    const Font* regFont = scene.uiFont.Get();
    const Font* boldFont = scene.uiBoldFont.Get();
    if (regFont == nullptr || !regFont->IsValid()) {
        return;
    }
    const bool boldAtlasOk =
            boldFont != nullptr && boldFont->IsValid() &&
            boldFont->GetAtlas().GetWidth() == regFont->GetAtlas().GetWidth() &&
            boldFont->GetAtlas().GetHeight() == regFont->GetAtlas().GetHeight();

    textScratchVertices.Clear();
    textScratchIndices.Clear();

    Array<std::size_t> drawOrder;
    StableSortTextDrawIndices(texts, drawOrder);

    std::size_t batchClipTextIndex = 0;
    bool haveTextBatch = false;

    constexpr VkDeviceSize kBytesPerTextGlyphVerts = sizeof(float) * 4U * 6U;
    constexpr VkDeviceSize kBytesPerTextGlyphIndices = sizeof(std::uint32_t) * 6U;
    constexpr std::size_t kMaxTextFloats = static_cast<std::size_t>(kTextVertexBytes / sizeof(float));
    constexpr std::size_t kMaxTextIndices = static_cast<std::size_t>(kTextIndexBytes / sizeof(std::uint32_t));

    for (std::size_t oi = 0; oi < drawOrder.GetSize(); ++oi) {
        const std::size_t li = drawOrder[oi];
        const ScreenTextDraw& td = texts[li];
        if (haveTextBatch && !VulkanScreenUiClip::SameTextBatch(texts[batchClipTextIndex], td)) {
            FlushAccumulatedUiTexts(
                    commandBuffer,
                    frameIndex,
                    extent,
                    texts[batchClipTextIndex],
                    PipelineForTextBlendMode(texts[batchClipTextIndex].blendMode));
            haveTextBatch = false;
        }
        if (!haveTextBatch) {
            batchClipTextIndex = li;
            haveTextBatch = true;
        }
        const std::size_t glyphEstimate = std::max<std::size_t>(1U, td.text.ByteLength());
        const std::size_t glyphMul = (td.bold && !boldAtlasOk) ? 3U : 1U;
        const std::size_t estFloats = glyphEstimate * glyphMul * 24U;
        const std::size_t estIndices = glyphEstimate * glyphMul * 6U;
        if (textScratchVertices.GetSize() + estFloats > kMaxTextFloats ||
            textScratchIndices.GetSize() + estIndices > kMaxTextIndices) {
            FlushAccumulatedUiTexts(
                    commandBuffer,
                    frameIndex,
                    extent,
                    texts[batchClipTextIndex],
                    PipelineForTextBlendMode(texts[batchClipTextIndex].blendMode));
            batchClipTextIndex = li;
            haveTextBatch = true;
        }
        AppendUiTextDrawGeometry(td, regFont, boldFont, boldAtlasOk);
        if (textScratchVertices.GetSize() > kMaxTextFloats || textScratchIndices.GetSize() > kMaxTextIndices) {
            FlushAccumulatedUiTexts(
                    commandBuffer,
                    frameIndex,
                    extent,
                    texts[batchClipTextIndex],
                    PipelineForTextBlendMode(texts[batchClipTextIndex].blendMode));
            haveTextBatch = false;
        }
    }
    if (haveTextBatch) {
        FlushAccumulatedUiTexts(
                commandBuffer,
                frameIndex,
                extent,
                texts[batchClipTextIndex],
                PipelineForTextBlendMode(texts[batchClipTextIndex].blendMode));
    }
}

void VulkanScreenUiPass::Record(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex,
        VkExtent2D extent,
        const SceneRenderParams& scene,
        bool sceneParamsValid) {
    if (!sceneParamsValid) {
        return;
    }

    VkRect2D fullFramebufferScissor{};
    fullFramebufferScissor.offset = {0, 0};
    fullFramebufferScissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &fullFramebufferScissor);

    const bool canSolid = PipelineForSolidBlendMode(kSceneBlendModeDefault) != VK_NULL_HANDLE;
    const bool canText = PipelineForTextBlendMode(kSceneBlendModeDefault) != VK_NULL_HANDLE &&
                         uploadedUiFont != nullptr && activeFontAtlas.view != VK_NULL_HANDLE &&
                         frameIndex < textDescriptorSets.GetSize();
    const Font* regFont = scene.uiFont.Get();
    const bool fontCpuOk = regFont != nullptr && regFont->IsValid();

    if (!canSolid && !(canText && fontCpuOk)) {
        return;
    }

    if (canSolid && !scene.screenRects.IsEmpty()) {
        RecordSolidRectsFor(commandBuffer, frameIndex, extent, scene.screenRects);
    }
    if (fontCpuOk && canText && !scene.screenTexts.IsEmpty()) {
        RecordTextOverlaysFor(commandBuffer, frameIndex, extent, scene, scene.screenTexts);
    }
    if (canSolid && !scene.screenOverlayRects.IsEmpty()) {
        RecordSolidRectsFor(commandBuffer, frameIndex, extent, scene.screenOverlayRects);
    }
    if (fontCpuOk && canText && !scene.screenOverlayTexts.IsEmpty()) {
        RecordTextOverlaysFor(commandBuffer, frameIndex, extent, scene, scene.screenOverlayTexts);
    }
    if (canSolid && !scene.screenLateRects.IsEmpty()) {
        RecordSolidRectsFor(commandBuffer, frameIndex, extent, scene.screenLateRects);
    }
    if (fontCpuOk && canText && !scene.screenLateTexts.IsEmpty()) {
        RecordTextOverlaysFor(commandBuffer, frameIndex, extent, scene, scene.screenLateTexts);
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullFramebufferScissor);
}

}  // namespace Spark
