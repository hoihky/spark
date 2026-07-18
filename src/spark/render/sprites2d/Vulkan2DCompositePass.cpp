#include "spark/render/sprites2d/Vulkan2DCompositePass.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/SpriteLighting2DComponent.hpp"
#include "spark/scene/DrawableSortKey.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"

namespace Spark {

namespace {

struct Composite2DEntry {
    bool isTilemap = false;
    std::size_t index = 0;
    std::int16_t sortingLayerOrder = 0;
    std::int32_t sortOrder = 0;
};

void StableSortCompositeEntries(Array<Composite2DEntry>& entries) noexcept {
    const auto moreInFront = [](const Composite2DEntry& a, const Composite2DEntry& b) noexcept -> bool {
        const DrawableSortKey keyA{a.sortingLayerOrder, a.sortOrder};
        const DrawableSortKey keyB{b.sortingLayerOrder, b.sortOrder};
        return DrawableSortMoreInFront(keyA, keyB);
    };
    const std::size_t n = entries.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        Composite2DEntry key = entries[i];
        std::size_t j = i;
        while (j > 0 && moreInFront(entries[j - 1], key)) {
            entries[j] = entries[j - 1];
            --j;
        }
        entries[j] = key;
    }
}

constexpr SceneBlendMode kBlendPassSequence[] = {
        SceneBlendMode::Multiply,
        SceneBlendMode::Opaque,
        SceneBlendMode::AlphaOver,
        SceneBlendMode::PremultipliedAlpha,
        SceneBlendMode::Screen,
        SceneBlendMode::Additive,
};

}  // namespace

void Vulkan2DCompositePass::Record(
        const VkCommandBuffer commandBuffer,
        const VulkanTilemapPass& tilemapPass,
        VulkanSpritePass& spritePass,
        const VulkanTilemapRecordContext& tilemapCtx,
        const VulkanSpriteRecordContext& spriteCtx) const {
    if (!tilemapCtx.sceneParamsValid || !spriteCtx.sceneParamsValid || tilemapCtx.scene == nullptr ||
        spriteCtx.scene == nullptr || tilemapCtx.vertexBuffer == VK_NULL_HANDLE ||
        tilemapCtx.indexBuffer == VK_NULL_HANDLE || tilemapCtx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }
    if (tilemapCtx.scene->tilemaps.IsEmpty() && tilemapCtx.scene->sprites.IsEmpty()) {
        return;
    }
    if (tilemapPass.GetPipelineLayout() == VK_NULL_HANDLE || spritePass.GetPipelineLayout() == VK_NULL_HANDLE) {
        return;
    }

    spritePass.ResetInstancing(spriteCtx.frameIndex);

    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tilemapCtx.vertexBuffer, &vbOff);
    vkCmdBindIndexBuffer(commandBuffer, tilemapCtx.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(tilemapCtx.extent.width);
    viewport.height = static_cast<float>(tilemapCtx.extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, tilemapCtx.scene, tilemapCtx.extent, fullScissor);

    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            tilemapPass.GetPipelineLayout(),
            0,
            1,
            &tilemapCtx.descriptorSet,
            0,
            nullptr);

    const SceneRenderParams& scene = *tilemapCtx.scene;
    Array<Composite2DEntry> entries;
    entries.Reserve(scene.tilemaps.GetSize() + scene.sprites.GetSize());

    Array<std::size_t> spriteBatchIndices;
    spriteBatchIndices.Reserve(256);

    auto flushSpriteBatch = [&](const SceneBlendMode blendMode) {
        if (spriteBatchIndices.IsEmpty()) {
            return;
        }
        spritePass.DrawSpritesBatched(
                commandBuffer,
                spriteCtx,
                scene,
                spriteBatchIndices.GetData(),
                spriteBatchIndices.GetSize(),
                blendMode);
        spriteBatchIndices.Clear();
    };

    for (const SceneBlendMode blendMode : kBlendPassSequence) {
        entries.Clear();
        for (std::size_t ti = 0; ti < scene.tilemaps.GetSize(); ++ti) {
            const SceneTilemapDraw& layer = scene.tilemaps[ti];
            if (layer.blendMode != blendMode || layer.textureLayer < 0 || layer.tileCount == 0) {
                continue;
            }
            Composite2DEntry entry{};
            entry.isTilemap = true;
            entry.index = ti;
            entry.sortingLayerOrder = layer.sortingLayerOrder;
            entry.sortOrder = layer.sortOrderBase;
            entries.PushBack(entry);
        }

        std::size_t spriteCount = scene.sprites.GetSize();
        if (spriteCount > static_cast<std::size_t>(SceneRenderParams::MaxSprites)) {
            spriteCount = static_cast<std::size_t>(SceneRenderParams::MaxSprites);
        }
        for (std::size_t si = 0; si < spriteCount; ++si) {
            const SceneSpriteDraw& sprite = scene.sprites[si];
            if (sprite.blendMode != blendMode) {
                continue;
            }
            Composite2DEntry entry{};
            entry.isTilemap = false;
            entry.index = si;
            entry.sortingLayerOrder = sprite.sortingLayerOrder;
            entry.sortOrder = sprite.sortOrder;
            entries.PushBack(entry);
        }

        StableSortCompositeEntries(entries);

        bool batchActive = false;
        std::int32_t batchTextureLayer = -2;
        auto batchLightingMode = SpriteLighting2DMode::None;

        for (std::size_t ei = 0; ei < entries.GetSize(); ++ei) {
            const Composite2DEntry& entry = entries[ei];
            if (entry.isTilemap) {
                flushSpriteBatch(blendMode);
                batchActive = false;
                tilemapPass.DrawLayer(
                        commandBuffer,
                        tilemapCtx,
                        scene.tilemaps[entry.index],
                        spritePass,
                        spriteCtx);
                continue;
            }

            const SceneSpriteDraw& sprite = scene.sprites[entry.index];
            if (!batchActive) {
                batchActive = true;
                batchTextureLayer = sprite.textureLayer;
                batchLightingMode = sprite.lightingMode;
                spriteBatchIndices.PushBack(entry.index);
                continue;
            }
            if (sprite.textureLayer == batchTextureLayer && sprite.lightingMode == batchLightingMode) {
                spriteBatchIndices.PushBack(entry.index);
            } else {
                flushSpriteBatch(blendMode);
                batchActive = true;
                batchTextureLayer = sprite.textureLayer;
                batchLightingMode = sprite.lightingMode;
                spriteBatchIndices.PushBack(entry.index);
            }
        }
        flushSpriteBatch(blendMode);
        batchActive = false;
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
