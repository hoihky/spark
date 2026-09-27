#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"

#include <cstdint>

namespace Spark {

/** Stable-sorts tile instances for draw order within one layer batch. */
class TilemapLayerSorter final {
public:
    void StableSortInstances(
            Array<SceneTilemapTileInstance>& tiles,
            std::uint32_t begin,
            std::uint32_t count,
            TilemapLayerSortMode mode,
            const Matrix4& worldTransform,
            float tileWorldSize) const noexcept;
};

}  // namespace Spark
