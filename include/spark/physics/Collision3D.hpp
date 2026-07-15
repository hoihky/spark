#pragma once

#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class BoxCollider3DComponent;
class SphereCollider3DComponent;

/** World-space axis-aligned box. */
struct CollisionAabb3 {
    float minX = 0.0F;
    float minY = 0.0F;
    float minZ = 0.0F;
    float maxX = 0.0F;
    float maxY = 0.0F;
    float maxZ = 0.0F;
};

/**
 * Static box entry for 3D broad-phase + narrow-phase. When <c>hasMaterial</c> is false, the simulator uses legacy
 * restitution from the dynamic rigidbody only and skips surface friction (matches pre-material behavior).
 */
struct StaticCollider3DSim {
    CollisionAabb3 aabb{};
    bool hasMaterial = false;
    float restitution = 0.0F;
    float staticFriction = 0.55F;
    float dynamicFriction = 0.48F;
};

[[nodiscard]] bool CollisionAabb3Overlaps(const CollisionAabb3& a, const CollisionAabb3& b) noexcept;

[[nodiscard]] bool CollisionAabb3OverlapsSphere(
        const CollisionAabb3& a, const Vector3& center, float radius) noexcept;

/** Same as <c>CollisionAabb3OverlapsSphere</c> but treats the sphere as <c>radius + inflateRadius</c> for the distance test. */
[[nodiscard]] bool CollisionAabb3OverlapsSphereInflated(
        const CollisionAabb3& a, const Vector3& center, float baseRadius, float inflateRadius) noexcept;

void ComputeBoxCollider3WorldAabb(
        GameObject& owner,
        const BoxCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept;

void ComputeSphereCollider3World(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        Vector3& outCenter,
        float& outRadius) noexcept;

/**
 * Static 3D colliders: no dynamic Rigidbody3D, or Rigidbody3D that is not Dynamic.
 */
[[nodiscard]] bool ContributesStaticCollider3D(GameObject& object) noexcept;

}  // namespace Spark
