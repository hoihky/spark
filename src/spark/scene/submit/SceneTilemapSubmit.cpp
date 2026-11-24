#include "spark/scene/submit/SceneTilemapSubmit.hpp"

#include "spark/scene/submit/Scene2DSubmitCaps.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"

#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp"
#include "spark/scene/submit/DrawableSortKey.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"
#include "spark/scene/tilemap/TilemapLayerSort.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void SceneTilemapSubmit::FillAtlasFields(const TilemapComponent& tilemap, SceneTilemapDraw& draw) noexcept {
    draw.atlasTilesU = tilemap.GetAtlasTilesU();
    draw.atlasTilesV = tilemap.GetAtlasTilesV();
    if (const SharedPtr<Tileset>& tileset = tilemap.GetTileset(); tileset) {
        draw.atlasMarginPixels = tileset->GetMarginPixels();
        draw.atlasSpacingPixels = tileset->GetSpacingPixels();
        draw.atlasTilePixelWidth = tileset->GetTilePixelWidth();
        draw.atlasTilePixelHeight = tileset->GetTilePixelHeight();
        if (const SharedPtr<Texture2D>& atlas = tileset->GetAtlas(); atlas) {
            draw.atlasLayerUvScaleU = atlas->GetSceneLayerUvScale().x;
            draw.atlasLayerUvScaleV = atlas->GetSceneLayerUvScale().y;
            if (tileset->GetImagePixelWidth() > 0U && tileset->GetImagePixelHeight() > 0U) {
                draw.atlasTextureWidth = tileset->GetImagePixelWidth();
                draw.atlasTextureHeight = tileset->GetImagePixelHeight();
            } else {
                draw.atlasTextureWidth = atlas->GetWidth();
                draw.atlasTextureHeight = atlas->GetHeight();
            }
        }
    }
}

void SceneTilemapSubmit::Submit(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSpriteTileCull& cull,
        const FindTextureLayerFn& findTextureLayer,
        const ResolveBlendModeFn& resolveBlendMode) const noexcept {
    params.tilemaps.Clear();
    params.tilemapTiles.Clear();
    params.tilemaps.Reserve(8);
    params.tilemapTiles.Reserve(512);

    world.Entities().ForEachActive([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        const TilemapComponent* tilemap = object->GetComponent<TilemapComponent>();
        if (tilemap == nullptr || !tilemap->GetAtlas() || tilemap->GetMapWidth() == 0 ||
            tilemap->GetMapHeight() == 0) {
            return;
        }
        const std::int32_t textureLayer = findTextureLayer(tilemap->GetAtlas());
        if (textureLayer < 0) {
            return;
        }

        const Matrix4 worldMatrix = object->GetWorldMatrix();
        const float tileWorldSize = tilemap->GetTileWorldSize();
        const SceneBlendMode blendMode = resolveBlendMode(*object);

        if (TilemapAutotileComponent* autotile = object->GetComponent<TilemapAutotileComponent>()) {
            autotile->RebuildIfNeeded(*object);
        }

        float tileAnimationTimeSeconds = 0.0F;
        if (const TilemapTileAnimatorComponent* animator = object->GetComponent<TilemapTileAnimatorComponent>()) {
            tileAnimationTimeSeconds = animator->GetAnimationTimeSeconds();
        }

        for (std::uint32_t layerIndex = 0; layerIndex < tilemap->GetLayerCount(); ++layerIndex) {
            if (!Scene2DSubmitCaps::HasTilemapDrawCapacity(params)) {
                Scene2DSubmitCaps::NotifyTilemapDrawCapacityReached();
                return;
            }

            const TilemapLayer& mapLayer = tilemap->GetLayer(layerIndex);
            if (!mapLayer.visible) {
                continue;
            }

            const std::uint32_t remainingTiles = Scene2DSubmitCaps::RemainingTilemapTiles(params);
            if (remainingTiles == 0) {
                Scene2DSubmitCaps::NotifyTilemapTileCapacityReached();
                return;
            }

            const std::uint32_t tileBegin = static_cast<std::uint32_t>(params.tilemapTiles.GetSize());
            const std::uint32_t appended = cull.CollectVisibleTiles(
                    worldMatrix,
                    tileWorldSize,
                    *tilemap,
                    layerIndex,
                    params.tilemapTiles,
                    remainingTiles,
                    tileAnimationTimeSeconds);
            if (appended == 0) {
                continue;
            }

            TilemapLayerSorter{}.StableSortInstances(
                    params.tilemapTiles,
                    tileBegin,
                    appended,
                    mapLayer.sortMode,
                    worldMatrix,
                    tileWorldSize);

            SceneTilemapDraw draw{};
            draw.worldTransform = worldMatrix;
            draw.tileWorldSize = tileWorldSize;
            FillAtlasFields(*tilemap, draw);
            draw.textureLayer = textureLayer;
            const std::int32_t nativeOrder = tilemap->GetSortOrderBase() + mapLayer.orderInLayerOffset;
            const ResolvedDrawableSort resolved = DrawableSortResolver::Resolve(*object, nativeOrder);
            draw.sortOrderBase = resolved.key.sortingOrder;
            draw.sortingLayerOrder = resolved.key.sortingLayerOrder;
            draw.sortWorldYAnchor = resolved.worldYAnchor;
            draw.blendMode = blendMode;
            draw.tileBegin = tileBegin;
            draw.tileCount = appended;
            draw.instanceSortMode = mapLayer.sortMode;
            params.tilemaps.PushBack(draw);
        }
    });

    SceneDrawListSorter::StableSortTilemaps(params.tilemaps);
}

}  // namespace Spark
