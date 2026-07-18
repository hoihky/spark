#include "spark/scene/SceneTilemapSubmit.hpp"

#include "spark/scene/DrawableSortKey.hpp"
#include "spark/scene/DrawableSortResolver.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cstdio>

namespace Spark {

void SceneTilemapSubmitter::StableSortTilemapDraws(Array<SceneTilemapDraw>& draws) noexcept {
    const auto moreInFront = [](const SceneTilemapDraw& a, const SceneTilemapDraw& b) noexcept -> bool {
        const DrawableSortKey keyA{a.sortingLayerOrder, a.sortOrderBase};
        const DrawableSortKey keyB{b.sortingLayerOrder, b.sortOrderBase};
        return DrawableSortMoreInFront(keyA, keyB);
    };
    const std::size_t n = draws.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneTilemapDraw key = draws[i];
        std::size_t j = i;
        while (j > 0 && moreInFront(draws[j - 1], key)) {
            draws[j] = draws[j - 1];
            --j;
        }
        draws[j] = key;
    }
}

void SceneTilemapSubmitter::Submit(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSpriteTileCull& cull,
        const FindTextureLayerFn& findTextureLayer,
        const ResolveBlendModeFn& resolveBlendMode) const noexcept {
    params.tilemaps.Clear();
    params.tilemapTiles.Clear();
    params.tilemaps.Reserve(8);
    params.tilemapTiles.Reserve(512);

    world.ForEachGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (params.tilemaps.GetSize() >= SceneRenderParams::MaxTilemapDraws) {
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

        const std::uint32_t remainingTiles =
                SceneRenderParams::MaxTilemapTiles -
                static_cast<std::uint32_t>(params.tilemapTiles.GetSize());
        if (remainingTiles == 0) {
            std::fprintf(stderr, "Spark: tilemap tile pool limit (%u) reached\n", SceneRenderParams::MaxTilemapTiles);
            return;
        }

        const std::uint32_t tileBegin = static_cast<std::uint32_t>(params.tilemapTiles.GetSize());
        const std::uint32_t appended = cull.CollectVisibleTiles(
                object->GetWorldMatrix(),
                tilemap->GetTileWorldSize(),
                *tilemap,
                params.tilemapTiles,
                remainingTiles);
        if (appended == 0) {
            return;
        }

        SceneTilemapDraw draw{};
        draw.worldTransform = object->GetWorldMatrix();
        draw.tileWorldSize = tilemap->GetTileWorldSize();
        draw.atlasTilesU = tilemap->GetAtlasTilesU();
        draw.atlasTilesV = tilemap->GetAtlasTilesV();
        draw.textureLayer = textureLayer;
        const ResolvedDrawableSort resolved =
                DrawableSortResolver::Resolve(*object, tilemap->GetSortOrderBase());
        draw.sortOrderBase = resolved.key.sortingOrder;
        draw.sortingLayerOrder = resolved.key.sortingLayerOrder;
        draw.blendMode = resolveBlendMode(*object);
        draw.tileBegin = tileBegin;
        draw.tileCount = appended;
        params.tilemaps.PushBack(draw);
    });

    StableSortTilemapDraws(params.tilemaps);
}

}  // namespace Spark
