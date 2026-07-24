#pragma once

#include <cstdint>

namespace Spark {

/** How visible tiles in one map layer are ordered before instanced draw (alpha overlap). */
enum class TilemapLayerSortMode : std::uint8_t {
    /** Row-major collection order (stable, fastest). */
    GridOrder = 0,
    /** Ascending world Y at each tile anchor (matches <c>SceneSpriteSortMode::SortOrderThenWorldY</c>). */
    WorldY = 1,
};

}  // namespace Spark
