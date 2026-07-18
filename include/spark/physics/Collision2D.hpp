#pragma once

#include <cstdint>

#include "spark/math/Vector2.hpp"

namespace Spark {

class GameObject;
class BoxCollider2DComponent;
class CircleCollider2DComponent;
class PolygonCollider2DComponent;

/** World-space axis-aligned box (XY; Z ignored). */
struct CollisionAabb2 {
    float minX = 0.0F;
    float minY = 0.0F;
    float maxX = 0.0F;
    float maxY = 0.0F;
};

enum class StaticCollider2DShape : std::uint8_t { Box = 0, Circle = 1, ConvexPolygon = 2 };

static constexpr std::uint32_t kMaxStaticPolygonVertices = 16;

/**
 * One static collider entry for 2D broad-phase (conservative `aabb`) and narrow-phase (`shape` + circle fields).
 */
struct StaticCollider2D {
    StaticCollider2DShape shape = StaticCollider2DShape::Box;
    CollisionAabb2 aabb{};
    float circleCx = 0.0F;
    float circleCy = 0.0F;
    float circleR = 0.0F;
    std::uint8_t polygonVertexCount = 0;
    float polygonVertsX[kMaxStaticPolygonVertices]{};
    float polygonVertsY[kMaxStaticPolygonVertices]{};
    /** Bitmask: which layer bits this static collider represents. */
    std::uint16_t categoryBits = 1u;
    /** Bitmask: which layer bits this static collider responds to. */
    std::uint16_t maskBits = 0xFFFFu;
    /** GameObject that owns this baked entry (for trigger signals). */
    GameObject* owner = nullptr;
    /** If true, no blocking — overlap only; dynamic bodies do not get position correction from this static. */
    bool isTrigger = false;
    bool hasMaterial = false;
    float restitution = 0.0F;
    float dynamicFriction = 0.48F;
};

[[nodiscard]] bool CollisionAabb2Overlaps(const CollisionAabb2& a, const CollisionAabb2& b) noexcept;

/** Solid disk vs solid axis-aligned rectangle (XY). */
[[nodiscard]] bool CollisionAabb2OverlapsCircle(const CollisionAabb2& a, float cx, float cy, float r) noexcept;

[[nodiscard]] bool CollisionCirclesOverlap(float ax, float ay, float ar, float bx, float by, float br) noexcept;

/**
 * Ray segment vs axis-aligned box (XY). Ray is <c>p(t) = (ox,oy) + t * (dx,dy)</c> for <c>t in [0, maxT]</c>.
 * Direction need not be normalized; returned <c>outT</c> is in the same units as <c>maxT</c> (distance if <c>(dx,dy)</c> is unit).
 * @return Closest hit distance along the ray, or false if no hit in range.
 */
[[nodiscard]] bool RaycastSegmentAabb2(
        float ox,
        float oy,
        float dx,
        float dy,
        float maxT,
        const CollisionAabb2& box,
        float& outT) noexcept;

/** Ray segment vs circle (XY). Same ray parameterization as <c>RaycastSegmentAabb2</c>. */
[[nodiscard]] bool RaycastSegmentCircle2(
        float ox,
        float oy,
        float dx,
        float dy,
        float maxT,
        float cx,
        float cy,
        float r,
        float& outT) noexcept;

[[nodiscard]] bool StaticCollider2DOverlapsWorldAabb(const StaticCollider2D& s, const CollisionAabb2& w) noexcept;

[[nodiscard]] bool StaticCollider2DOverlapsWorldCircle(
        const StaticCollider2D& s, float cx, float cy, float r) noexcept;

void ComputeBoxCollider2WorldAabb(
        GameObject& owner,
        const BoxCollider2DComponent& collider,
        CollisionAabb2& outWorld) noexcept;

void ComputeCircleCollider2World(
        GameObject& owner,
        const CircleCollider2DComponent& collider,
        float& outCx,
        float& outCy,
        float& outR) noexcept;

/** Fills world-space convex polygon vertices and conservative AABB. Requires at least 3 vertices. */
void ComputePolygonCollider2DWorld(
        GameObject& owner,
        const PolygonCollider2DComponent& collider,
        StaticCollider2D& outStatic) noexcept;

[[nodiscard]] bool CollisionConvexPolygonOverlapsWorldAabb(const StaticCollider2D& poly, const CollisionAabb2& box)
        noexcept;

[[nodiscard]] bool CollisionConvexPolygonOverlapsWorldCircle(
        const StaticCollider2D& poly,
        float cx,
        float cy,
        float r) noexcept;

/**
 * Minimum translation to separate a world AABB from a convex polygon (static).
 * Returns false when not overlapping.
 */
[[nodiscard]] bool TryComputeBoxPolygonSeparation(
        const CollisionAabb2& box,
        const StaticCollider2D& poly,
        float& outNx,
        float& outNy,
        float& outPenetration) noexcept;

[[nodiscard]] bool TryComputeCirclePolygonSeparation(
        float cx,
        float cy,
        float cr,
        const StaticCollider2D& poly,
        float& outNx,
        float& outNy,
        float& outPenetration) noexcept;

/**
 * True when this object can contribute static 2D colliders (tilemap, box, and/or circle): has at least one of
 * those components, and either no rigidbody or a non-dynamic rigidbody.
 */
[[nodiscard]] bool ContributesStaticCollider2D(GameObject& object) noexcept;

/** True when the object has a box collider and it is static per rigidbody rules (ignores circle-only bodies). */
[[nodiscard]] bool IsStaticBoxCollider2D(GameObject& object) noexcept;

}  // namespace Spark
