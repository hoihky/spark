#include "spark/render/sprites2d/VulkanSpritePass.hpp"

#include "spark/render/gpu/VulkanBlendAttachment.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"

#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

void FillSpriteInstanceGpu(const SceneSpriteDraw& sprite, VulkanSpriteInstanceGpu& out) noexcept {
    std::memcpy(out.model, sprite.model.m, sizeof(out.model));
    out.tint[0] = sprite.tint.x;
    out.tint[1] = sprite.tint.y;
    out.tint[2] = sprite.tint.z;
    out.tint[3] = sprite.tint.w;
    out.uvRect[0] = sprite.uvRect.x;
    out.uvRect[1] = sprite.uvRect.y;
    out.uvRect[2] = sprite.uvRect.z;
    out.uvRect[3] = sprite.uvRect.w;
    out.textureLayer = sprite.textureLayer;
    out.lightingMode = static_cast<std::int32_t>(sprite.lightingMode);
    out.lightingPad0 = sprite.lightingPad0;
    out.lightingPad1 = sprite.lightingPad1;
    out.lightingA[0] = sprite.lightingParam0.x;
    out.lightingA[1] = sprite.lightingParam0.y;
    out.lightingA[2] = sprite.lightingParam0.z;
    out.lightingA[3] = sprite.lightingParam0.w;
    out.lightingB[0] = sprite.lightingParam1.x;
    out.lightingB[1] = sprite.lightingParam1.y;
    out.lightingB[2] = sprite.lightingParam1.z;
    out.lightingB[3] = sprite.lightingParam1.w;
}

}  // namespace

void VulkanSpritePass::CreateGpuResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const std::uint32_t framesInFlight) {
    DestroyGpuResources(device);
    instanceBuffers.Resize(framesInFlight);
    instanceMemory.Resize(framesInFlight);
    instanceMapped.Resize(framesInFlight);
    instanceWriteCursor.Resize(framesInFlight);
    for (std::uint32_t i = 0; i < framesInFlight; ++i) {
        instanceWriteCursor[i] = 0;
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                static_cast<VkDeviceSize>(kQuadInstanceSsboBytes),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                instanceBuffers[i],
                instanceMemory[i]);
        if (vkMapMemory(
                    device,
                    instanceMemory[i],
                    0,
                    static_cast<VkDeviceSize>(kQuadInstanceSsboBytes),
                    0,
                    &instanceMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory sprite instance SSBO failed");
        }
    }
}

void VulkanSpritePass::DestroyGpuResources(const VkDevice device) {
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
    instanceWriteCursor.Clear();
}

VkBuffer VulkanSpritePass::InstanceBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= instanceBuffers.GetSize()) {
        return VK_NULL_HANDLE;
    }
    return instanceBuffers[frameIndex];
}

void VulkanSpritePass::ResetInstancing(const std::uint32_t frameIndex) noexcept {
    if (frameIndex < instanceWriteCursor.GetSize()) {
        instanceWriteCursor[frameIndex] = 0;
    }
}

void VulkanSpritePass::DestroyGraphicsPipeline(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t i = 0; i < kSceneBlendModeCount; ++i) {
        if (pipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipelines[i], nullptr);
            pipelines[i] = VK_NULL_HANDLE;
        }
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

VkPipeline VulkanSpritePass::PipelineForBlendMode(const SceneBlendMode mode) const noexcept {
    const std::size_t index = static_cast<std::size_t>(mode);
    if (index >= kSceneBlendModeCount) {
        return pipelines[static_cast<std::size_t>(kSceneBlendModeDefault)];
    }
    const VkPipeline pipe = pipelines[index];
    if (pipe != VK_NULL_HANDLE) {
        return pipe;
    }
    return pipelines[static_cast<std::size_t>(kSceneBlendModeDefault)];
}

void VulkanSpritePass::CreateGraphicsPipeline(
        const VkDevice device,
        const VkRenderPass hdrRenderPass,
        const VkDescriptorSetLayout sceneDescriptorSetLayout,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE ||
        sceneDescriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    const Array<char> sv = shaders.ReadSpvFile("sprite.vert.spv");
    const Array<char> sf = shaders.ReadSpvFile("sprite.frag.spv");
    vertModule = shaders.CreateShaderModule(sv);
    fragModule = shaders.CreateShaderModule(sf);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    const VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    using VL = VulkanSceneVertexLayout;
    constexpr std::uint32_t kStride = VL::kStrideBytes;
    VkVertexInputBindingDescription bind{};
    bind.binding = 0;
    bind.stride = kStride;
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[3]{};
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

    VkPipelineVertexInputStateCreateInfo vtxIn{};
    vtxIn.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vtxIn.vertexBindingDescriptionCount = 1;
    vtxIn.pVertexBindingDescriptions = &bind;
    vtxIn.vertexAttributeDescriptionCount = 3;
    vtxIn.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.lineWidth = 1.0F;
    rast.cullMode = VK_CULL_MODE_NONE;
    rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = kVulkanBlendColorWriteMaskRgb;
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    const VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2u;
    dyn.pDynamicStates = dynStates;

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(SpriteBatchPushConstants);

    VkPipelineLayoutCreateInfo pl{};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &sceneDescriptorSetLayout;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(device, &pl, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (sprite) failed");
    }

    VkGraphicsPipelineCreateInfo pipe{};
    pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe.stageCount = 2;
    pipe.pStages = stages;
    pipe.pVertexInputState = &vtxIn;
    pipe.pInputAssemblyState = &ia;
    pipe.pViewportState = &vp;
    pipe.pRasterizationState = &rast;
    pipe.pMultisampleState = &ms;
    pipe.pDepthStencilState = &ds;
    pipe.pColorBlendState = &blend;
    pipe.pDynamicState = &dyn;
    pipe.layout = pipelineLayout;
    pipe.renderPass = hdrRenderPass;
    pipe.subpass = 0;

    for (std::size_t mi = 0; mi < kSceneBlendModeCount; ++mi) {
        const auto mode = static_cast<SceneBlendMode>(mi);
        if (VulkanCreateGraphicsPipelineForBlendMode(device, pipe, mode, &pipelines[mi]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipelines (sprite blend) failed");
        }
    }
}

bool VulkanSpritePass::WriteInstances(
        const std::uint32_t frameIndex,
        const SceneRenderParams& scene,
        const std::size_t* spriteIndices,
        const std::size_t spriteCount,
        std::uint32_t& outInstanceBase) const {
    outInstanceBase = 0;
    if (spriteCount == 0 || frameIndex >= instanceMapped.GetSize() || spriteIndices == nullptr ||
        frameIndex >= instanceWriteCursor.GetSize()) {
        return false;
    }
    void* mapped = instanceMapped[frameIndex];
    if (mapped == nullptr) {
        return false;
    }

    outInstanceBase = instanceWriteCursor[frameIndex];
    if (outInstanceBase + spriteCount > kMaxQuadInstancesGpu) {
        outInstanceBase = 0;
        return false;
    }

    auto* base = static_cast<VulkanSpriteInstanceGpu*>(mapped);
    VulkanSpriteInstanceGpu* dst = base + outInstanceBase;
    for (std::size_t i = 0; i < spriteCount; ++i) {
        const std::size_t si = spriteIndices[i];
        if (si >= scene.sprites.GetSize()) {
            continue;
        }
        FillSpriteInstanceGpu(scene.sprites[si], dst[i]);
    }
    instanceWriteCursor[frameIndex] = outInstanceBase + static_cast<std::uint32_t>(spriteCount);
    return true;
}

bool VulkanSpritePass::ReserveQuadInstances(
        const std::uint32_t frameIndex,
        const std::uint32_t count,
        std::uint32_t& outInstanceBase,
        VulkanSpriteInstanceGpu*& outInstances) const {
    outInstanceBase = 0;
    outInstances = nullptr;
    if (count == 0 || frameIndex >= instanceMapped.GetSize() || frameIndex >= instanceWriteCursor.GetSize()) {
        return false;
    }
    void* mapped = instanceMapped[frameIndex];
    if (mapped == nullptr) {
        return false;
    }

    outInstanceBase = instanceWriteCursor[frameIndex];
    if (outInstanceBase + count > kMaxQuadInstancesGpu) {
        return false;
    }

    outInstances = static_cast<VulkanSpriteInstanceGpu*>(mapped) + outInstanceBase;
    instanceWriteCursor[frameIndex] = outInstanceBase + count;
    return true;
}

void VulkanSpritePass::DrawInstancedQuads(
        const VkCommandBuffer commandBuffer,
        const VulkanSpriteRecordContext& ctx,
        const std::uint32_t instanceBase,
        const std::uint32_t instanceCount,
        const SceneBlendMode blendMode) const {
    if (pipelineLayout == VK_NULL_HANDLE || instanceCount == 0) {
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineForBlendMode(blendMode));

    const SpriteBatchPushConstants pc{.instanceBase = instanceBase};
    vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SpriteBatchPushConstants),
            &pc);
    vkCmdDrawIndexed(commandBuffer, ctx.quadIndexCount, instanceCount, ctx.quadFirstIndex, 0, 0);
}

void VulkanSpritePass::DrawSpritesBatched(
        const VkCommandBuffer commandBuffer,
        const VulkanSpriteRecordContext& ctx,
        const SceneRenderParams& scene,
        const std::size_t* spriteIndices,
        const std::size_t spriteCount,
        const SceneBlendMode blendMode) const {
    if (pipelineLayout == VK_NULL_HANDLE || spriteCount == 0 || spriteIndices == nullptr) {
        return;
    }

    std::uint32_t instanceBase = 0;
    if (!WriteInstances(ctx.frameIndex, scene, spriteIndices, spriteCount, instanceBase)) {
        return;
    }

    DrawInstancedQuads(commandBuffer, ctx, instanceBase, static_cast<std::uint32_t>(spriteCount), blendMode);
}

void VulkanSpritePass::DrawSprite(
        const VkCommandBuffer commandBuffer,
        const VulkanSpriteRecordContext& ctx,
        const SceneSpriteDraw& sprite) const {
    if (ctx.scene == nullptr) {
        return;
    }
    std::size_t index = 0;
    for (std::size_t i = 0; i < ctx.scene->sprites.GetSize(); ++i) {
        if (&ctx.scene->sprites[i] == &sprite) {
            index = i;
            DrawSpritesBatched(commandBuffer, ctx, *ctx.scene, &index, 1, sprite.blendMode);
            return;
        }
    }
    SceneRenderParams temp{};
    temp.sprites.PushBack(sprite);
    DrawSpritesBatched(commandBuffer, ctx, temp, &index, 1, sprite.blendMode);
}

void VulkanSpritePass::Record(const VkCommandBuffer commandBuffer, const VulkanSpriteRecordContext& ctx) const {
    if (!ctx.sceneParamsValid || pipelineLayout == VK_NULL_HANDLE || ctx.vertexBuffer == VK_NULL_HANDLE ||
        ctx.indexBuffer == VK_NULL_HANDLE || ctx.scene == nullptr || ctx.scene->sprites.IsEmpty() ||
        ctx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }

    std::size_t n = ctx.scene->sprites.GetSize();
    if (n > static_cast<std::size_t>(SceneRenderParams::MaxSprites)) {
        n = static_cast<std::size_t>(SceneRenderParams::MaxSprites);
    }

    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.vertexBuffer, &vbOff);
    vkCmdBindIndexBuffer(commandBuffer, ctx.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(ctx.extent.width);
    viewport.height = static_cast<float>(ctx.extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);

    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    Array<std::size_t> batchIndices;
    batchIndices.Reserve(n);
    SceneBlendMode batchBlend = ctx.scene->sprites[0].blendMode;
    std::int32_t batchTexture = ctx.scene->sprites[0].textureLayer;
    auto lightingMode = ctx.scene->sprites[0].lightingMode;

    auto flushBatch = [&]() {
        if (batchIndices.IsEmpty()) {
            return;
        }
        DrawSpritesBatched(commandBuffer, ctx, *ctx.scene, batchIndices.GetData(), batchIndices.GetSize(), batchBlend);
        batchIndices.Clear();
    };

    for (std::size_t i = 0; i < n; ++i) {
        const SceneSpriteDraw& s = ctx.scene->sprites[i];
        if (s.blendMode != batchBlend || s.textureLayer != batchTexture || s.lightingMode != lightingMode) {
            flushBatch();
            batchBlend = s.blendMode;
            batchTexture = s.textureLayer;
            lightingMode = s.lightingMode;
        }
        batchIndices.PushBack(i);
    }
    flushBatch();

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
