#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;

/** When <c>Physics2DTriggerOverlap</c> involves two dynamics, <c>payload.b</c> equals this (no static collider index). */
inline constexpr std::uint32_t kPhysics2DTriggerOverlapNoStaticIndex = 0xFFFFFFFFu;

/**
 * Rebuilt static broad-phase (same data as <c>SimulatePhysics2D</c> uses): <c>StaticCollider2D</c> array +
 * <c>SpatialHashGrid2D</c>. Call <c>Rebuild</c> once per frame (or step) then run multiple queries without
 * re-walking the world for statics.
 */
struct StaticBroadPhase2D {
    Array<StaticCollider2D> statics;
    SpatialHashGrid2D grid;

    /** Default cell size matches <c>SimulatePhysics2D</c> (4 world units). */
    void Rebuild(GameWorld& world, float cellWorldSize = 4.0F);
};

/**
 * Layer filtering for queries: uses the same rule as simulation
 * (<c>CollisionFilter2D::ShouldCollide</c>(queryCategory, queryMask, static.category, static.mask)).
 *
 * - <c>hitSolids</c> / <c>hitTriggers</c>: narrow-phase hits are skipped unless the static collider matches.
 * - Queries **do not** emit <c>SignalId::Physics2DTriggerOverlap</c>; that signal is reserved for the physics step.
 */
struct PhysicsQueryFilter2D {
    std::uint16_t queryCategoryBits = CollisionFilter2D::DefaultCategory();
    std::uint16_t queryMaskBits = CollisionFilter2D::AllLayersMask();
    bool hitSolids = true;
    bool hitTriggers = true;
};

struct PhysicsQueryHit2D {
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
};

/** Hit from a **dynamic** rigidbody query (no baked static index). */
struct PhysicsQueryHitDynamic2D {
    GameObject* owner = nullptr;
};

struct PhysicsRaycastHit2D {
    float distanceAlongRay = 0.0F;
    float hitX = 0.0F;
    float hitY = 0.0F;
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
};

/**
 * Overlap tests against the **static** broad-phase (same bake as simulation). <c>outHits</c> is cleared first.
 * <c>centerX</c>/<c>centerY</c>/<c>radius</c> define a world-space circle query volume.
 */
void QueryOverlapCircleStatics2D(
        const StaticBroadPhase2D& broadPhase,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits);

/** World-space axis-aligned rectangle overlap against statics. <c>outHits</c> is cleared first. */
void QueryOverlapAabbStatics2D(
        const StaticBroadPhase2D& broadPhase,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits);

/**
 * Raycast against static colliders. <c>dirX</c>/<c>dirY</c> should be a **unit** direction; <c>maxDistance</c> is in
 * world units. Returns false with <c>outHit</c> untouched if no hit; otherwise fills the closest hit along the ray.
 */
bool RaycastStatics2D(
        const StaticBroadPhase2D& broadPhase,
        float originX,
        float originY,
        float dirX,
        float dirY,
        float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit);

/** Convenience: rebuilds static broad-phase then runs <c>QueryOverlapCircleStatics2D</c>. */
void QueryOverlapCircleWorld2D(
        GameWorld& world,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        float cellWorldSize = 4.0F);

/** Convenience: rebuild + AABB overlap. */
void QueryOverlapAabbWorld2D(
        GameWorld& world,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        float cellWorldSize = 4.0F);

/** Convenience: rebuild + raycast. */
bool RaycastWorld2D(
        GameWorld& world,
        float originX,
        float originY,
        float dirX,
        float dirY,
        float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit,
        float cellWorldSize = 4.0F);

/**
 * Overlap all **dynamic** rigidbodies whose primary collider intersects a world-space circle.
 * Uses the same broad-phase cell size and layer rules as <c>SimulatePhysics2D</c> dynamic pairs (P0.4).
 * <c>outHits</c> is cleared first. <c>ignore</c> may be null.
 */
void QueryOverlapCircleDynamics2D(
        GameWorld& world,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        float cellWorldSize = 4.0F);

/**
 * Arc / cone attack: same broad-phase disk as <c>QueryOverlapCircleStatics2D</c>, then narrows to a symmetric
 * angular sector around <c>(dirX, dirY)</c> (normalized internally). Targets are included if their collider
 * overlaps the disk **and** a conservative sector test on the collider center (with radius slack) passes.
 */
void QueryOverlapArcStatics2D(
        const StaticBroadPhase2D& broadPhase,
        float originX,
        float originY,
        float radius,
        float dirX,
        float dirY,
        float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits);

void QueryOverlapArcDynamics2D(
        GameWorld& world,
        float originX,
        float originY,
        float radius,
        float dirX,
        float dirY,
        float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        float cellWorldSize = 4.0F);

void QueryOverlapArcWorldStatics2D(
        GameWorld& world,
        float originX,
        float originY,
        float radius,
        float dirX,
        float dirY,
        float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        float cellWorldSize = 4.0F);

void QueryOverlapArcWorldDynamics2D(
        GameWorld& world,
        float originX,
        float originY,
        float radius,
        float dirX,
        float dirY,
        float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        float cellWorldSize = 4.0F);

}  // namespace Spark
