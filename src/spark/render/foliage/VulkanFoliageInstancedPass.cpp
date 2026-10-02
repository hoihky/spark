#include "spark/render/foliage/VulkanFoliageInstancedPass.hpp"

#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/foliage/VulkanFoliageInstanceGpu.hpp"
#include "spark/render/scene/VulkanCustomMeshPool.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"
#include "spark/scene/foliage/SceneFoliageInstancedBatch.hpp"

#include <cstring>
#include <stdexcept>

namespace Spark {

void VulkanFoliageInstancedPass::CreateGpuResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const std::uint32_t framesInFlight) {
    DestroyGpuResources(device);
    instanceBuffers.Resize(framesInFlight);
    instanceMemory.Resize(framesInFlight);
    instanceMapped.Resize(framesInFlight);
    for (std::uint32_t i = 0; i < framesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                static_cast<VkDeviceSize>(kFoliageInstanceSsboBytes),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                instanceBuffers[i],
                instanceMemory[i]);
        if (vkMapMemory(
                    device,
                    instanceMemory[i],
                    0,
                    static_cast<VkDeviceSize>(kFoliageInstanceSsboBytes),
                    0,
                    &instanceMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory foliage instance SSBO failed");
        }
    }
}

void VulkanFoliageInstancedPass::DestroyGpuResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t i = 0; i < instanceBuffers.GetSize(); ++i) {
        if (instanceMapped[i] != nullptr) {
            vkUnmapMemory(device, instanceMemory[i]);
            instanceMapped[i] = nullptr;
        }
        if (instanceBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, instanceBuffers[i], nullptr);
            instanceBuffers[i] = VK_NULL_HANDLE;
        }
        if (instanceMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, instanceMemory[i], nullptr);
            instanceMemory[i] = VK_NULL_HANDLE;
        }
    }
    instanceBuffers.Clear();
    instanceMemory.Clear();
    instanceMapped.Clear();
}

VkBuffer VulkanFoliageInstancedPass::InstanceBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= instanceBuffers.GetSize()) {
        return VK_NULL_HANDLE;
    }
    return instanceBuffers[frameIndex];
}

void VulkanFoliageInstancedPass::DestroyGraphicsPipeline(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (vertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vertModule = VK_NULL_HANDLE;
    }
    if (fragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, fragModule, nullptr);
        fragModule = VK_NULL_HANDLE;
    }
}

void VulkanFoliageInstancedPass::CreateGraphicsPipeline(
        const VkDevice device,
        const VkRenderPass hdrRenderPass,
        const VkDescriptorSetLayout sceneDescriptorSetLayout,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE || sceneDescriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    const Array<char> vertSpv = shaders.ReadSpvFile("foliage.vert.spv");
    const Array<char> fragSpv = shaders.ReadSpvFile("foliage.frag.spv");
    vertModule = shaders.CreateShaderModule(vertSpv);
    fragModule = shaders.CreateShaderModule(fragSpv);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    using VL = VulkanSceneVertexLayout;
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = VL::kStrideBytes;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributes[3]{};
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = sizeof(float) * VL::kOffPosition;
    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = sizeof(float) * VL::kOffNormal;
    attributes[2].location = 2;
    attributes[2].binding = 0;
    attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[2].offset = sizeof(float) * VL::kOffTexCoord0;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 3;
    vertexInput.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttach;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(FoliageBatchPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &sceneDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("VulkanFoliageInstancedPass: vkCreatePipelineLayout failed");
    }

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertexInput;
    pipeInfo.pInputAssemblyState = &inputAssembly;
    pipeInfo.pViewportState = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState = &ms;
    pipeInfo.pDepthStencilState = &depth;
    pipeInfo.pColorBlendState = &blend;
    pipeInfo.pDynamicState = &dynamic;
    pipeInfo.layout = pipelineLayout;
    pipeInfo.renderPass = hdrRenderPass;
    pipeInfo.subpass = 0;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("VulkanFoliageInstancedPass: vkCreateGraphicsPipelines failed");
    }
}

bool VulkanFoliageInstancedPass::UploadInstances(
        const std::uint32_t frameIndex,
        const SceneRenderParams& scene) const {
    if (frameIndex >= instanceMapped.GetSize() || instanceMapped[frameIndex] == nullptr) {
        return false;
    }
    const std::size_t count = scene.foliageInstances.GetSize();
    if (count == 0) {
        return true;
    }
    if (count > static_cast<std::size_t>(kMaxFoliageInstancesGpu)) {
        return false;
    }
    auto* dst = static_cast<VulkanFoliageInstanceGpu*>(instanceMapped[frameIndex]);
    for (std::size_t i = 0; i < count; ++i) {
        const FoliageInstanceRecord& src = scene.foliageInstances[i];
        std::memcpy(dst[i].model, src.GetModelMatrix().m, sizeof(dst[i].model));
        const Vector4 tint = src.GetTint();
        dst[i].tint[0] = tint.x;
        dst[i].tint[1] = tint.y;
        dst[i].tint[2] = tint.z;
        dst[i].tint[3] = tint.w;
        dst[i].windPhase = src.GetWindPhase();
    }
    return true;
}

void VulkanFoliageInstancedPass::Record(
        const VkCommandBuffer commandBuffer,
        const VulkanFoliageRecordContext& ctx) const {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || pipeline == VK_NULL_HANDLE ||
        ctx.descriptorSet == VK_NULL_HANDLE || ctx.customMeshPool == nullptr) {
        return;
    }
    if (ctx.scene->foliageBatches.IsEmpty() || ctx.scene->foliageInstances.IsEmpty()) {
        return;
    }
    if (ctx.customVertexBuffer == VK_NULL_HANDLE || ctx.customIndexBuffer == VK_NULL_HANDLE) {
        return;
    }
    if (!UploadInstances(ctx.frameIndex, *ctx.scene)) {
        return;
    }

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);
    VulkanScreenUiClip::BindScenePassViewport(commandBuffer, ctx.scene, ctx.extent);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.customVertexBuffer, &vbOff);
    vkCmdBindIndexBuffer(commandBuffer, ctx.customIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    const Array<SceneFoliageInstancedBatch>& batches = ctx.scene->foliageBatches;
    for (std::size_t bi = 0; bi < batches.GetSize(); ++bi) {
        const SceneFoliageInstancedBatch& batch = batches[bi];
        const Mesh* mesh = batch.GetSourceMesh().Get();
        if (mesh == nullptr || batch.GetInstanceCount() == 0U) {
            continue;
        }
        const CustomMeshGpuSlice slice = ctx.customMeshPool->TryGetRigidMeshSlice(mesh);
        if (slice.indexCount == 0U) {
            continue;
        }
        FoliageBatchPushConstants push{};
        push.instanceBase = batch.GetInstanceBegin();
        push.textureLayer = batch.GetTextureLayer();
        push.albedoTint[0] = batch.GetAlbedoTint().x;
        push.albedoTint[1] = batch.GetAlbedoTint().y;
        push.albedoTint[2] = batch.GetAlbedoTint().z;
        push.albedoTint[3] = 1.0F;
        push.alphaCutoff = batch.GetAlphaCutoff();
        push.bladeHeight = batch.GetBladeHeight();
        push.windBendScale = batch.GetWindBendScale();
        push.fadeStartDistance = batch.GetFadeStartDistanceMeters();
        push.fadeEndDistance = batch.GetFadeEndDistanceMeters();
        vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(FoliageBatchPushConstants),
                &push);
        vkCmdDrawIndexed(
                commandBuffer,
                slice.indexCount,
                batch.GetInstanceCount(),
                slice.firstIndex,
                slice.vertexOffset,
                0);
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
