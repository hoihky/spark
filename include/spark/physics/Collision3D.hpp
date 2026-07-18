#pragma once

#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class BoxCollider3DComponent;
class CapsuleCollider3DComponent;
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

/** World-space capsule: segment from <c>pointA</c> to <c>pointB</c> with uniform <c>radius</c>. */
struct CollisionCapsule3 {
    Vector3 pointA{Vector3::Zero};
    Vector3 pointB{Vector3::Zero};
    float radius = 0.5F;
};

enum class StaticCollider3DShape : std::uint8_t {
    Box = 0,
    Capsule = 1,
};

/**
 * Static collider entry for 3D broad-phase + narrow-phase. <c>aabb</c> always stores the world envelope used by the
 * spatial hash. When <c>shape</c> is <c>Capsule</c>, <c>capsule</c> holds the narrow-phase geometry.
 */
struct StaticCollider3DSim {
    StaticCollider3DShape shape = StaticCollider3DShape::Box;
    CollisionAabb3 aabb{};
    CollisionCapsule3 capsule{};
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

/** Builds world capsule endpoints and radius from a <c>CapsuleCollider3DComponent</c>. */
void ComputeCapsuleCollider3World(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        CollisionCapsule3& outWorld) noexcept;

/** Tight world AABB around a capsule (for broad-phase insertion). */
void ComputeCapsuleCollider3WorldAabb(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept;

/** Sphere vs axis-aligned box contact (closest-point normal). Optional slop inflates overlap test. */
[[nodiscard]] bool ComputeSphereAabbContact(
        const Vector3& center,
        float radius,
        const CollisionAabb3& box,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

/** Sphere vs capsule contact (closest-point on segment normal). Normal points from capsule toward sphere. */
[[nodiscard]] bool ComputeSphereCapsuleContact(
        const Vector3& center,
        float radius,
        const CollisionCapsule3& capsule,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

/** Capsule vs axis-aligned box. Normal points from box toward capsule. */
[[nodiscard]] bool ComputeCapsuleAabbContact(
        const CollisionCapsule3& capsule,
        const CollisionAabb3& box,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

/** Capsule vs capsule. Normal points from <c>a</c> toward <c>b</c>. */
[[nodiscard]] bool ComputeCapsuleCapsuleContact(
        const CollisionCapsule3& a,
        const CollisionCapsule3& b,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

/** Dispatches to box or capsule narrow-phase contact for a static collider entry. */
[[nodiscard]] bool ComputeSphereStaticCollider3Contact(
        const Vector3& center,
        float radius,
        const StaticCollider3DSim& collider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

/** Conservative overlap test for a sphere against a static collider entry. */
[[nodiscard]] bool StaticCollider3DOverlapsSphere(
        const StaticCollider3DSim& collider, const Vector3& center, float radius) noexcept;

/** Pushes <c>center</c> out of <c>box</c> along the contact normal when penetrating. */
[[nodiscard]] bool SeparateSphereFromAabb(Vector3& center, float radius, const CollisionAabb3& box) noexcept;

/** Pushes <c>center</c> out of a static collider along the contact normal when penetrating. */
[[nodiscard]] bool SeparateSphereFromStaticCollider3(
        Vector3& center, float radius, const StaticCollider3DSim& collider) noexcept;

/**
 * Static 3D colliders: <c>BoxCollider3DComponent</c> and/or <c>CapsuleCollider3DComponent</c> on objects without a
 * dynamic <c>Rigidbody3DComponent</c>. Objects with <c>CharacterController3DComponent</c> are never static colliders.
 */
[[nodiscard]] bool ContributesStaticCollider3D(GameObject& object) noexcept;

}  // namespace Spark
