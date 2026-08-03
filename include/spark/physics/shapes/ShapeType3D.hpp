#pragma once

#include <cstdint>

namespace Spark {

enum class ShapeType3D : std::uint8_t {
    Box = 0,
    Sphere = 1,
    Capsule = 2,
};

inline constexpr std::uint32_t kShape3DTypeCount = 3;

}  // namespace Spark
