#pragma once

#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

enum class VolumeShape : std::uint8_t {
    Box = 0,
    Sphere = 1,
};

/** Tests whether a world point lies inside an oriented box or sphere volume on <c>owner</c>. */
[[nodiscard]] bool PointInsideVolume(
        const Vector3& worldPoint,
        const GameObject& owner,
        VolumeShape shape,
        const Vector3& halfExtents) noexcept;

}  // namespace Spark
