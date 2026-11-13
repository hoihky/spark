#pragma once

#include <cstdint>

namespace Spark {

enum class ShapeType2D : std::uint8_t {
    Box = 0,
    Circle = 1,
    ConvexPolygon = 2,
};

inline constexpr std::uint32_t kShape2DTypeCount = 3;

}  // namespace Spark
