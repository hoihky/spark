#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"

#include <cstdint>

namespace Spark {

/** Stable-sorts a slice of <c>SceneRenderParams::tilemapTiles</c> for draw order within one layer batch. */
void StableSortTilemapTileInstances(
        Array<SceneTilemapTileInstance>& tiles,
        std::uint32_t begin,
        std::uint32_t count,
        TilemapLayerSortMode mode,
        const Matrix4& worldTransform,
        float tileWorldSize) noexcept;

}  // namespace Spark
