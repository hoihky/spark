#include "spark/scene/VolumeRegions.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"

#include <cmath>

namespace Spark {

bool PointInsideVolume(
        const Vector3& worldPoint,
        const GameObject& owner,
        const VolumeShape shape,
        const Vector3& halfExtents) noexcept {
    Matrix4 inv{};
    if (!owner.GetWorldMatrix().TryInvert(inv)) {
        return false;
    }
    const Vector3 local = inv.TransformPoint(worldPoint);
    if (shape == VolumeShape::Sphere) {
        const float r = std::max(halfExtents.x, 1.0e-4F);
        return local.LengthSquared() <= r * r;
    }
    return std::abs(local.x) <= halfExtents.x && std::abs(local.y) <= halfExtents.y &&
           std::abs(local.z) <= halfExtents.z;
}

}  // namespace Spark
