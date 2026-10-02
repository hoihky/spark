#include "spark/scene/foliage/GrassFieldBounds.hpp"

#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

void GrassFieldBounds::SetHalfExtentsMeters(const float halfX, const float halfZ) noexcept {
    halfExtentX = halfX > 0.5F ? halfX : 0.5F;
    halfExtentZ = halfZ > 0.5F ? halfZ : 0.5F;
}

bool GrassFieldBounds::ContainsWorldXZ(
        const Vector3& fieldOriginWorld,
        const float worldX,
        const float worldZ) const noexcept {
    const float dx = worldX - fieldOriginWorld.x;
    const float dz = worldZ - fieldOriginWorld.z;
    return std::fabs(dx) <= halfExtentX && std::fabs(dz) <= halfExtentZ;
}

}  // namespace Spark
