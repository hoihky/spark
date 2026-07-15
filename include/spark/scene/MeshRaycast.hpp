#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class Mesh;

/**
 * Ray vs indexed triangle mesh in world space. Returns the nearest hit with <c>t</c> along the ray
 * (<c>ro + rd * t</c>), or false if no hit in <c>[tMin, tMax]</c>.
 */
[[nodiscard]] bool TryRaycastMeshWorld(
        const Vector3& rayOriginWorld,
        const Vector3& rayDirWorld,
        const Mesh& mesh,
        const Matrix4& worldFromLocal,
        float tMin,
        float tMax,
        float& outT) noexcept;

/** Ray vs unit sphere centered at <c>centerWorld</c> with given radius. */
[[nodiscard]] bool TryRaycastSphereWorld(
        const Vector3& rayOriginWorld,
        const Vector3& rayDirWorld,
        const Vector3& centerWorld,
        float radius,
        float tMin,
        float tMax,
        float& outT) noexcept;

}  // namespace Spark
