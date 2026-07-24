#include "spark/render/sprites2d/VulkanTilemapPass.hpp"

#include "spark/render/gpu/VulkanBlendAttachment.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"
#include "spark/render/sprites2d/VulkanSpritePass.hpp"
#include "spark/scene/SceneTileAtlas.hpp"
#include "spark/scene/tilemap/TileTransform.hpp"

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

void FillTileInstanceGpu(
        const SceneTilemapDraw& layer,
        const SceneTilemapTileInstance& tile,
        VulkanSpriteInstanceGpu& out) noexcept {
    Vector4 uv{};
    TileIdToAtlasUvRect(
            tile.tileId,
            layer.atlasTilesU,
            layer.atlasTilesV,
            layer.atlasMarginPixels,
            layer.atlasSpacingPixels,
            layer.atlasTextureWidth,
            layer.atlasTextureHeight,
            layer.atlasTilePixelWidth,
            layer.atlasTilePixelHeight,
            uv);

    const float ts = layer.tileWorldSize;
    const float flipH =
            (tile.transformFlags & static_cast<std::uint8_t>(TileTransformFlags::FlipH)) != 0 ? -1.0F : 1.0F;
    const float flipV =
            (tile.transformFlags & static_cast<std::uint8_t>(TileTransformFlags::FlipV)) != 0 ? -1.0F : 1.0F;
    const std::uint8_t rotCount = TileTransformRotation90Count(tile.transformFlags);
    const float angleRad = static_cast<float>(rotCount) * (3.14159265F * 0.5F);
    const Matrix4 rotZ = Matrix4::Rotation(Quaternion::FromAxisAngle(Vector3::UnitZ, angleRad));

    const Matrix4 tileModel =
            layer.worldTransform *
            Matrix4::Translation(
                    {(static_cast<float>(tile.gridX) + tile.anchorNormX) * ts,
                     (static_cast<float>(tile.gridY) + tile.anchorNormY) * ts,
                     0.0F}) *
            rotZ * Matrix4::Scale({ts * flipH, ts * flipV, 1.0F});

    std::memcpy(out.model, tileModel.m, sizeof(out.model));
    const float inv255 = 1.0F / 255.0F;
    out.tint[0] = static_cast<float>(tile.tintR) * inv255;
    out.tint[1] = static_cast<float>(tile.tintG) * inv255;
    out.tint[2] = static_cast<float>(tile.tintB) * inv255;
    out.tint[3] = static_cast<float>(tile.tintA) * inv255;
    out.uvRect[0] = uv.x;
    out.uvRect[1] = uv.y;
    out.uvRect[2] = uv.z;
    out.uvRect[3] = uv.w;
    out.textureLayer = layer.textureLayer;
    out.lightingMode = 0;
    out.lightingPad0 = 0.0F;
    out.lightingPad1 = 0.0F;
    out.lightingA[0] = 1.0F;
    out.lightingA[1] = 1.0F;
    out.lightingA[2] = 1.0F;
    out.lightingA[3] = 1.0F;
    out.lightingB[0] = 1.0F;
    out.lightingB[1] = 0.0F;
    out.lightingB[2] = 0.0F;
    out.lightingB[3] = 0.0F;
}

}  // namespace

void VulkanTilemapPass::DestroyGraphicsPipeline(const VkDevice device) {
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

VkPipeline VulkanTilemapPass::PipelineForBlendMode(const SceneBlendMode mode) const noexcept {
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

void VulkanTilemapPass::CreateGraphicsPipeline(
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

    struct TileBatchPushConstants {
        std::uint32_t instanceBase = 0;
    };

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(TileBatchPushConstants);

    VkPipelineLayoutCreateInfo pl{};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &sceneDescriptorSetLayout;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(device, &pl, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (tilemap) failed");
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
            throw std::runtime_error("vkCreateGraphicsPipelines (tilemap blend) failed");
        }
    }
}

void VulkanTilemapPass::DrawLayer(
        const VkCommandBuffer commandBuffer,
        const VulkanTilemapRecordContext& ctx,
        const SceneTilemapDraw& layer,
        VulkanSpritePass& instancing,
        const VulkanSpriteRecordContext& spriteCtx) const {
    if (ctx.scene == nullptr || layer.textureLayer < 0 || layer.tileCount == 0) {
        return;
    }
    const Array<SceneTilemapTileInstance>& tiles = ctx.scene->tilemapTiles;
    const std::size_t tileEnd = static_cast<std::size_t>(layer.tileBegin) + static_cast<std::size_t>(layer.tileCount);
    if (tileEnd > tiles.GetSize()) {
        return;
    }

    std::uint32_t tileOffset = layer.tileBegin;
    std::uint32_t tilesRemaining = layer.tileCount;
    while (tilesRemaining > 0) {
        VulkanSpriteInstanceGpu* dst = nullptr;
        std::uint32_t instanceBase = 0;
        std::uint32_t chunkCount = tilesRemaining;
        while (chunkCount > 0U &&
               !instancing.ReserveQuadInstances(spriteCtx.frameIndex, chunkCount, instanceBase, dst)) {
            chunkCount /= 2U;
        }
        if (chunkCount == 0U || dst == nullptr) {
            return;
        }

        for (std::uint32_t i = 0; i < chunkCount; ++i) {
            const SceneTilemapTileInstance& tile = tiles[static_cast<std::size_t>(tileOffset + i)];
            FillTileInstanceGpu(layer, tile, dst[i]);
        }

        instancing.DrawInstancedQuads(commandBuffer, spriteCtx, instanceBase, chunkCount, layer.blendMode);
        tileOffset += chunkCount;
        tilesRemaining -= chunkCount;
    }
}

void VulkanTilemapPass::Record(
        const VkCommandBuffer commandBuffer,
        const VulkanTilemapRecordContext& ctx,
        VulkanSpritePass& instancing,
        const VulkanSpriteRecordContext& spriteCtx) const {
    if (!ctx.sceneParamsValid || pipelineLayout == VK_NULL_HANDLE || ctx.vertexBuffer == VK_NULL_HANDLE ||
        ctx.indexBuffer == VK_NULL_HANDLE || ctx.scene == nullptr || ctx.scene->tilemaps.IsEmpty() ||
        ctx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }

    instancing.ResetInstancing(spriteCtx.frameIndex);

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

    const Array<SceneTilemapDraw>& layers = ctx.scene->tilemaps;
    for (std::size_t li = 0; li < layers.GetSize(); ++li) {
        DrawLayer(commandBuffer, ctx, layers[li], instancing, spriteCtx);
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
