#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/scene/tilemap/TileTransform.hpp"

#include <cmath>
#include <cstdint>

namespace Spark {

/** Maps a point in normalized tile cell space [0,1]² through flip + 90° CCW steps (around cell center). */
[[nodiscard]] inline Vector2 TransformNormalizedCellPoint(const Vector2& p, const std::uint8_t transformFlags) noexcept {
    float x = p.x - 0.5F;
    float y = p.y - 0.5F;
    if ((transformFlags & static_cast<std::uint8_t>(TileTransformFlags::FlipH)) != 0) {
        x = -x;
    }
    if ((transformFlags & static_cast<std::uint8_t>(TileTransformFlags::FlipV)) != 0) {
        y = -y;
    }
    const std::uint8_t rot = TileTransformRotation90Count(transformFlags);
    for (std::uint8_t i = 0; i < rot; ++i) {
        const float nx = -y;
        const float ny = x;
        x = nx;
        y = ny;
    }
    return {x + 0.5F, y + 0.5F};
}

}  // namespace Spark
