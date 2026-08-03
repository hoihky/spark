#include "spark/physics/PhysicsQueries2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/physics/colliders/DynamicBody2D.hpp"
#include "spark/physics/colliders/DynamicCollider2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

namespace {

[[nodiscard]] bool PassesQueryFilter(
        const PhysicsQueryFilter2D& filter,
        const Collider2D& col) noexcept {
    return col.GetFilter().PassesQueryFilter(
            filter.queryCategoryBits, filter.queryMaskBits, filter.hitSolids, filter.hitTriggers);
}

[[nodiscard]] bool RaycastStaticCollider2D(
        const Collider2D& col,
        const float ox,
        const float oy,
        const float dx,
        const float dy,
        const float maxT,
        float& outT) noexcept {
    const StaticCollider2D snap = col.ToLegacySnapshot();
    if (snap.shape == StaticCollider2DShape::Box) {
        return RaycastSegmentAabb2(ox, oy, dx, dy, maxT, snap.aabb, outT);
    }
    if (snap.shape == StaticCollider2DShape::Circle) {
        return RaycastSegmentCircle2(ox, oy, dx, dy, maxT, snap.circleCx, snap.circleCy, snap.circleR, outT);
    }
    return RaycastSegmentAabb2(ox, oy, dx, dy, maxT, snap.aabb, outT);
}

}  // namespace

void QueryOverlapCircleStatics2D(
        const BroadPhase2D& broadPhase,
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) {
    outHits.Clear();
    if (radius < 0.0F) {
        return;
    }

    CollisionAabb2 query{};
    query.minX = centerX - radius;
    query.maxX = centerX + radius;
    query.minY = centerY - radius;
    query.maxY = centerY + radius;

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(query, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider2D& col = broadPhase.GetColliders()[si];
        if (!PassesQueryFilter(filter, col)) {
            continue;
        }
        if (!col.OverlapsCircle(centerX, centerY, radius)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = col.GetOwner();
        outHits.PushBack(h);
    }
}

void QueryOverlapAabbStatics2D(
        const BroadPhase2D& broadPhase,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) {
    outHits.Clear();

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(worldAabb, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider2D& col = broadPhase.GetColliders()[si];
        if (!PassesQueryFilter(filter, col)) {
            continue;
        }
        if (!col.OverlapsAabb(worldAabb)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = col.GetOwner();
        outHits.PushBack(h);
    }
}

bool RaycastStatics2D(
        const BroadPhase2D& broadPhase,
        const float originX,
        const float originY,
        const float dirX,
        const float dirY,
        const float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit) {
    if (maxDistance <= 0.0F) {
        return false;
    }

    const float endX = originX + dirX * maxDistance;
    const float endY = originY + dirY * maxDistance;
    CollisionAabb2 sweep{};
    sweep.minX = (std::min)(originX, endX);
    sweep.maxX = (std::max)(originX, endX);
    sweep.minY = (std::min)(originY, endY);
    sweep.maxY = (std::max)(originY, endY);
    constexpr float kPad = 1.0e-3F;
    sweep.minX -= kPad;
    sweep.maxX += kPad;
    sweep.minY -= kPad;
    sweep.maxY += kPad;

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(sweep, candidates);

    float bestT = std::numeric_limits<float>::infinity();
    bool any = false;
    PhysicsRaycastHit2D best{};

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider2D& col = broadPhase.GetColliders()[si];
        if (!PassesQueryFilter(filter, col)) {
            continue;
        }
        float t = 0.0F;
        if (!RaycastStaticCollider2D(col, originX, originY, dirX, dirY, maxDistance, t)) {
            continue;
        }
        if (t < bestT) {
            bestT = t;
            best.distanceAlongRay = t;
            best.hitX = originX + dirX * t;
            best.hitY = originY + dirY * t;
            best.staticColliderIndex = si;
            best.owner = col.GetOwner();
            any = true;
        }
    }

    if (!any) {
        return false;
    }
    outHit = best;
    return true;
}

void QueryOverlapCircleWorld2D(
        GameWorld& world,
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        const float cellWorldSize) {
    BroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    QueryOverlapCircleStatics2D(bp, centerX, centerY, radius, filter, outHits);
}

void QueryOverlapAabbWorld2D(
        GameWorld& world,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        const float cellWorldSize) {
    BroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    QueryOverlapAabbStatics2D(bp, worldAabb, filter, outHits);
}

bool RaycastWorld2D(
        GameWorld& world,
        const float originX,
        const float originY,
        const float dirX,
        const float dirY,
        const float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit,
        const float cellWorldSize) {
    BroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    return RaycastStatics2D(bp, originX, originY, dirX, dirY, maxDistance, filter, outHit);
}

namespace {

[[nodiscard]] bool OverlapWorldCircleWithDynamicBody(
        const float qx,
        const float qy,
        const float qr,
        DynamicBody2D& body) noexcept {
    RefreshDynamicBody2D(body);
    return body.collider.OverlapsCircle(qx, qy, qr);
}

void DynamicBodyCenterAndSlack(DynamicBody2D& body, float& cx, float& cy, float& slack) noexcept {
    RefreshDynamicBody2D(body);
    const CollisionAabb2 bounds = body.collider.GetBounds();
    cx = 0.5F * (bounds.minX + bounds.maxX);
    cy = 0.5F * (bounds.minY + bounds.maxY);
    const float dx = bounds.maxX - bounds.minX;
    const float dy = bounds.maxY - bounds.minY;
    slack = 0.5F * std::sqrt(dx * dx + dy * dy);
}

[[nodiscard]] bool PassesDynamicQueryFilter(
        const PhysicsQueryFilter2D& filter,
        const bool bodyIsTrigger,
        const std::uint16_t bodyCat,
        const std::uint16_t bodyMask) noexcept {
    if (!CollisionFilter2D::ShouldCollide(
                filter.queryCategoryBits,
                filter.queryMaskBits,
                bodyCat,
                bodyMask)) {
        return false;
    }
    if (bodyIsTrigger && !filter.hitTriggers) {
        return false;
    }
    if (!bodyIsTrigger && !filter.hitSolids) {
        return false;
    }
    return true;
}

[[nodiscard]] bool RoughCenterInFiniteSector(
        const float cx,
        const float cy,
        const float slack,
        const float ox,
        const float oy,
        const float rdx,
        const float rdy,
        const float cosHalfAngle,
        const float maxReach) noexcept {
    const float vx = cx - ox;
    const float vy = cy - oy;
    const float len2 = vx * vx + vy * vy;
    constexpr float kEps2 = 1.0e-12F;
    if (len2 <= kEps2) {
        return true;
    }
    const float len = std::sqrt(len2);
    if (len > maxReach + slack) {
        return false;
    }
    const float inv = 1.0F / len;
    const float c = (vx * rdx + vy * rdy) * inv;
    return c >= cosHalfAngle;
}

[[nodiscard]] bool NormalizeDir2D(const float dirX, const float dirY, float& outX, float& outY) noexcept {
    const float d2 = dirX * dirX + dirY * dirY;
    constexpr float kEps2 = 1.0e-12F;
    if (d2 <= kEps2) {
        return false;
    }
    const float inv = 1.0F / std::sqrt(d2);
    outX = dirX * inv;
    outY = dirY * inv;
    return true;
}

[[nodiscard]] bool StaticRoughInArcSector(
        const Collider2D& col,
        const float ox,
        const float oy,
        const float rdx,
        const float rdy,
        const float cosHalfAngle,
        const float maxReach) noexcept {
    const StaticCollider2D snap = col.ToLegacySnapshot();
    float cx = 0.0F;
    float cy = 0.0F;
    float slack = 0.0F;
    if (snap.shape == StaticCollider2DShape::Circle) {
        cx = snap.circleCx;
        cy = snap.circleCy;
        slack = snap.circleR;
    } else {
        cx = 0.5F * (snap.aabb.minX + snap.aabb.maxX);
        cy = 0.5F * (snap.aabb.minY + snap.aabb.maxY);
        const float dx = snap.aabb.maxX - snap.aabb.minX;
        const float dy = snap.aabb.maxY - snap.aabb.minY;
        slack = 0.5F * std::sqrt(dx * dx + dy * dy);
    }
    return RoughCenterInFiniteSector(cx, cy, slack, ox, oy, rdx, rdy, cosHalfAngle, maxReach);
}

void QueryDynamicsWithCircleAndOptionalSector(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        const float cellWorldSize,
        const bool useSector,
        const float sectorDirX,
        const float sectorDirY,
        const float cosHalfAngle) {
    outHits.Clear();
    if (radius < 0.0F) {
        return;
    }

    float rdx = 1.0F;
    float rdy = 0.0F;
    if (useSector && !NormalizeDir2D(sectorDirX, sectorDirY, rdx, rdy)) {
        return;
    }

    Array<DynamicBody2D> bodies;
    CollectDynamicBodies2D(world, bodies);
    const std::size_t n = bodies.GetSize();
    if (n == 0) {
        return;
    }

    SpatialHashGrid2D dynBroad;
    dynBroad.Clear();
    const float cell = (cellWorldSize > 1.0e-4F) ? cellWorldSize : 4.0F;
    dynBroad.SetCellSize(cell);

    for (std::size_t i = 0; i < n; ++i) {
        RefreshDynamicBody2D(bodies[i]);
        dynBroad.InsertIndexedAabb(static_cast<std::uint32_t>(i), bodies[i].collider.GetBounds());
    }

    CollisionAabb2 query{};
    query.minX = originX - radius;
    query.maxX = originX + radius;
    query.minY = originY - radius;
    query.maxY = originY + radius;

    Array<std::uint32_t> candidates;
    dynBroad.QueryUniquePayloadIndices(query, candidates);

    for (std::size_t k = 0; k < candidates.GetSize(); ++k) {
        const std::uint32_t bi = candidates[k];
        if (bi >= n) {
            continue;
        }
        DynamicBody2D& body = bodies[bi];
        if (ignore != nullptr && body.object == ignore) {
            continue;
        }
        const ColliderFilter& bodyFilter = body.collider.GetFilter();
        if (!PassesDynamicQueryFilter(
                    filter, bodyFilter.isTrigger, bodyFilter.categoryBits, bodyFilter.maskBits)) {
            continue;
        }
        if (!OverlapWorldCircleWithDynamicBody(originX, originY, radius, body)) {
            continue;
        }
        if (useSector) {
            float rcx = 0.0F;
            float rcy = 0.0F;
            float slack = 0.0F;
            DynamicBodyCenterAndSlack(body, rcx, rcy, slack);
            if (!RoughCenterInFiniteSector(rcx, rcy, slack, originX, originY, rdx, rdy, cosHalfAngle, radius)) {
                continue;
            }
        }
        PhysicsQueryHitDynamic2D h{};
        h.owner = body.object;
        outHits.PushBack(h);
    }
}

}  // namespace

void QueryOverlapCircleDynamics2D(
        GameWorld& world,
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        const float cellWorldSize) {
    QueryDynamicsWithCircleAndOptionalSector(
            world, centerX, centerY, radius, filter, ignore, outHits, cellWorldSize, false, 0.0F, 0.0F, 0.0F);
}

void QueryOverlapArcDynamics2D(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        const float cellWorldSize) {
    const float cosHalf = std::cos(halfAngleRadians);
    QueryDynamicsWithCircleAndOptionalSector(
            world,
            originX,
            originY,
            radius,
            filter,
            ignore,
            outHits,
            cellWorldSize,
            true,
            dirX,
            dirY,
            cosHalf);
}

void QueryOverlapArcStatics2D(
        const BroadPhase2D& broadPhase,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) {
    outHits.Clear();
    if (radius < 0.0F) {
        return;
    }
    float rdx = 1.0F;
    float rdy = 0.0F;
    if (!NormalizeDir2D(dirX, dirY, rdx, rdy)) {
        return;
    }
    const float cosHalf = std::cos(halfAngleRadians);

    CollisionAabb2 query{};
    query.minX = originX - radius;
    query.maxX = originX + radius;
    query.minY = originY - radius;
    query.maxY = originY + radius;

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(query, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider2D& col = broadPhase.GetColliders()[si];
        if (!PassesQueryFilter(filter, col)) {
            continue;
        }
        if (!col.OverlapsCircle(originX, originY, radius)) {
            continue;
        }
        if (!StaticRoughInArcSector(col, originX, originY, rdx, rdy, cosHalf, radius)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = col.GetOwner();
        outHits.PushBack(h);
    }
}

void QueryOverlapArcWorldStatics2D(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        const float cellWorldSize) {
    BroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    QueryOverlapArcStatics2D(bp, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, outHits);
}

void QueryOverlapArcWorldDynamics2D(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits,
        const float cellWorldSize) {
    QueryOverlapArcDynamics2D(
            world, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, ignore, outHits, cellWorldSize);
}

void PhysicsQueryWorld2D::RebuildStatics(GameWorld& world) {
    broadPhase.Rebuild(world, cellWorldSize);
}

void PhysicsQueryWorld2D::OverlapCircleStatics(
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) const {
    QueryOverlapCircleStatics2D(broadPhase, centerX, centerY, radius, filter, outHits);
}

void PhysicsQueryWorld2D::OverlapAabbStatics(
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) const {
    QueryOverlapAabbStatics2D(broadPhase, worldAabb, filter, outHits);
}

bool PhysicsQueryWorld2D::RaycastStatics(
        const float originX,
        const float originY,
        const float dirX,
        const float dirY,
        const float maxDistance,
        const PhysicsQueryFilter2D& filter,
        PhysicsRaycastHit2D& outHit) const {
    return RaycastStatics2D(broadPhase, originX, originY, dirX, dirY, maxDistance, filter, outHit);
}

void PhysicsQueryWorld2D::OverlapArcStatics(
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) const {
    QueryOverlapArcStatics2D(
            broadPhase, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, outHits);
}

void PhysicsQueryWorld2D::OverlapCircleDynamics(
        GameWorld& world,
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits) const {
    QueryOverlapCircleDynamics2D(world, centerX, centerY, radius, filter, ignore, outHits, cellWorldSize);
}

void PhysicsQueryWorld2D::OverlapArcDynamics(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic2D>& outHits) const {
    QueryOverlapArcDynamics2D(
            world, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, ignore, outHits, cellWorldSize);
}

}  // namespace Spark
