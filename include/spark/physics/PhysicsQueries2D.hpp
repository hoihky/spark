#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/BroadPhase2D.hpp"
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
 * Object-oriented 2D physics query service. Rebuilds (or reuses) a static broad-phase, then runs overlap and
 * raycast queries without re-walking the ECS for statics on every call.
 */
class PhysicsQueryWorld2D {
public:
    explicit PhysicsQueryWorld2D(float cellWorldSizeIn = kBroadPhase2DDefaultCellSize)
            : cellWorldSize(cellWorldSizeIn) {}

    /** Rebuilds the internal static broad-phase from the current ECS state. */
    void RebuildStatics(GameWorld& world);

    [[nodiscard]] const BroadPhase2D& GetBroadPhase() const noexcept { return broadPhase; }
    [[nodiscard]] BroadPhase2D& GetBroadPhase() noexcept { return broadPhase; }

    void SetCellWorldSize(float cellWorldSizeIn) noexcept { cellWorldSize = cellWorldSizeIn; }
    [[nodiscard]] float GetCellWorldSize() const noexcept { return cellWorldSize; }

    void OverlapCircleStatics(
            float centerX,
            float centerY,
            float radius,
            const PhysicsQueryFilter2D& filter,
            Array<PhysicsQueryHit2D>& outHits) const;

    void OverlapAabbStatics(
            const CollisionAabb2& worldAabb,
            const PhysicsQueryFilter2D& filter,
            Array<PhysicsQueryHit2D>& outHits) const;

    [[nodiscard]] bool RaycastStatics(
            float originX,
            float originY,
            float dirX,
            float dirY,
            float maxDistance,
            const PhysicsQueryFilter2D& filter,
            PhysicsRaycastHit2D& outHit) const;

    void OverlapArcStatics(
            float originX,
            float originY,
            float radius,
            float dirX,
            float dirY,
            float halfAngleRadians,
            const PhysicsQueryFilter2D& filter,
            Array<PhysicsQueryHit2D>& outHits) const;

    void OverlapCircleDynamics(
            GameWorld& world,
            float centerX,
            float centerY,
            float radius,
            const PhysicsQueryFilter2D& filter,
            GameObject* ignore,
            Array<PhysicsQueryHitDynamic2D>& outHits) const;

    void OverlapArcDynamics(
            GameWorld& world,
            float originX,
            float originY,
            float radius,
            float dirX,
            float dirY,
            float halfAngleRadians,
            const PhysicsQueryFilter2D& filter,
            GameObject* ignore,
            Array<PhysicsQueryHitDynamic2D>& outHits) const;

private:
    BroadPhase2D broadPhase{};
    float cellWorldSize = 4.0F;
};

// --- Free-function query API (backward compatible) ---

void QueryOverlapCircleStatics2D(
        const BroadPhase2D& broadPhase,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits);

void QueryOverlapAabbStatics2D(
        const BroadPhase2D& broadPhase,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits);

bool RaycastStatics2D(
        const BroadPhase2D& broadPhase,
        float originX,
        float originY,
        float dirX,
        float dirY,
        float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit);

void QueryOverlapCircleWorld2D(
        GameWorld& world,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        float cellWorldSize = 4.0F);

void QueryOverlapAabbWorld2D(
        GameWorld& world,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        float cellWorldSize = 4.0F);

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

void QueryOverlapCircleDynamics2D(
        GameWorld& world,
        float centerX,
        float centerY,
        float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        float cellWorldSize = 4.0F);

void QueryOverlapArcStatics2D(
        const BroadPhase2D& broadPhase,
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
