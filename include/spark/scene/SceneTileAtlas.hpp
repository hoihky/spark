#pragma once

#include "spark/ecs/components/TilemapComponent.hpp"
#include "spark/math/Vector4.hpp"

#include <cstdint>

namespace Spark {

/** Maps a tile id into normalized atlas UV bounds (minU, minV, maxU, maxV). */
void TileIdToAtlasUvRect(
        std::uint16_t tileId,
        std::uint32_t atlasTilesU,
        std::uint32_t atlasTilesV,
        Vector4& outUv) noexcept;

}  // namespace Spark
