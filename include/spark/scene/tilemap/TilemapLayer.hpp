#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"

#include <cstdint>

namespace Spark {

/**
 * One logical map layer inside a <c>TilemapComponent</c> (Tiled-style stacking).
 * All layers share the parent map dimensions and tileset.
 */
struct TilemapLayer {
    Utf8String name{};
    Array<TileCell> cells{};
    /** Added to the owning component's <c>sortOrderBase</c> when resolving draw order. */
    std::int32_t orderInLayerOffset = 0;
    bool visible = true;
    /** When false, tiles are not baked into <c>TilemapCollider2D</c>. */
    bool contributeCollision = true;
    /** When false, layer is ignored when baking <c>TilemapGameplayGrid</c>. */
    bool contributeGameplayGrid = true;
    TilemapLayerSortMode sortMode = TilemapLayerSortMode::GridOrder;

    [[nodiscard]] static TilemapLayer MakeDefault() noexcept {
        TilemapLayer layer{};
        layer.name = Utf8String("Layer 0");
        return layer;
    }
};

}  // namespace Spark
