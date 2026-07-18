#include "spark/physics/PhysicsQueries2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

void StaticBroadPhase2D::Rebuild(GameWorld& world, const float cellWorldSize) {
    RebuildBroadPhaseFromStaticColliders2D(world, cellWorldSize, statics, grid);
}

namespace {

[[nodiscard]] bool PassesQueryFilter(
        const PhysicsQueryFilter2D& filter,
        const StaticCollider2D& st) noexcept {
    if (!CollisionFilter2D::ShouldCollide(
                filter.queryCategoryBits,
                filter.queryMaskBits,
                st.categoryBits,
                st.maskBits)) {
        return false;
    }
    if (st.isTrigger && !filter.hitTriggers) {
        return false;
    }
    if (!st.isTrigger && !filter.hitSolids) {
        return false;
    }
    return true;
}

[[nodiscard]] bool RaycastStaticCollider2D(
        const StaticCollider2D& st,
        const float ox,
        const float oy,
        const float dx,
        const float dy,
        const float maxT,
        float& outT) noexcept {
    if (st.shape == StaticCollider2DShape::Box) {
        return RaycastSegmentAabb2(ox, oy, dx, dy, maxT, st.aabb, outT);
    }
    return RaycastSegmentCircle2(ox, oy, dx, dy, maxT, st.circleCx, st.circleCy, st.circleR, outT);
}

}  // namespace

void QueryOverlapCircleStatics2D(
        const StaticBroadPhase2D& broadPhase,
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
    broadPhase.grid.QueryUniquePayloadIndices(query, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.statics.GetSize()) {
            continue;
        }
        const StaticCollider2D& st = broadPhase.statics[si];
        if (!PassesQueryFilter(filter, st)) {
            continue;
        }
        if (!StaticCollider2DOverlapsWorldCircle(st, centerX, centerY, radius)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = st.owner;
        outHits.PushBack(h);
    }
}

void QueryOverlapAabbStatics2D(
        const StaticBroadPhase2D& broadPhase,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits) {
    outHits.Clear();

    Array<std::uint32_t> candidates;
    broadPhase.grid.QueryUniquePayloadIndices(worldAabb, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.statics.GetSize()) {
            continue;
        }
        const StaticCollider2D& st = broadPhase.statics[si];
        if (!PassesQueryFilter(filter, st)) {
            continue;
        }
        if (!StaticCollider2DOverlapsWorldAabb(st, worldAabb)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = st.owner;
        outHits.PushBack(h);
    }
}

bool RaycastStatics2D(
        const StaticBroadPhase2D& broadPhase,
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
    broadPhase.grid.QueryUniquePayloadIndices(sweep, candidates);

    float bestT = std::numeric_limits<float>::infinity();
    bool any = false;
    PhysicsRaycastHit2D best{};

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.statics.GetSize()) {
            continue;
        }
        const StaticCollider2D& st = broadPhase.statics[si];
        if (!PassesQueryFilter(filter, st)) {
            continue;
        }
        float t = 0.0F;
        if (!RaycastStaticCollider2D(st, originX, originY, dirX, dirY, maxDistance, t)) {
            continue;
        }
        if (t < bestT) {
            bestT = t;
            best.distanceAlongRay = t;
            best.hitX = originX + dirX * t;
            best.hitY = originY + dirY * t;
            best.staticColliderIndex = si;
            best.owner = st.owner;
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
    StaticBroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    QueryOverlapCircleStatics2D(bp, centerX, centerY, radius, filter, outHits);
}

void QueryOverlapAabbWorld2D(
        GameWorld& world,
        const CollisionAabb2& worldAabb,
        const PhysicsQueryFilter2D& filter,
        Array<PhysicsQueryHit2D>& outHits,
        const float cellWorldSize) {
    StaticBroadPhase2D bp;
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
    StaticBroadPhase2D bp;
    bp.Rebuild(world, cellWorldSize);
    return RaycastStatics2D(bp, originX, originY, dirX, dirY, maxDistance, filter, outHit);
}

namespace {

struct DynamicQueryBody2D {
    GameObject* object = nullptr;
    BoxCollider2DComponent* box = nullptr;
    CircleCollider2DComponent* circle = nullptr;
};

void GatherDynamicQueryBodies2D(GameWorld& world, Array<DynamicQueryBody2D>& out) {
    out.Clear();
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        Rigidbody2DComponent* rb = o->GetComponent<Rigidbody2DComponent>();
        TransformComponent* tr = o->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType2D::Dynamic) {
            return;
        }
        BoxCollider2DComponent* boxCol = o->GetComponent<BoxCollider2DComponent>();
        CircleCollider2DComponent* circleCol = o->GetComponent<CircleCollider2DComponent>();
        if (circleCol == nullptr && boxCol == nullptr) {
            return;
        }
        DynamicQueryBody2D b{};
        b.object = o;
        b.box = boxCol;
        b.circle = circleCol;
        out.PushBack(b);
    });
}

void ComputeDynamicQueryBodyAabb(const DynamicQueryBody2D& body, CollisionAabb2& out) noexcept {
    if (body.circle != nullptr) {
        float cx = 0.0F;
        float cy = 0.0F;
        float cr = 0.0F;
        ComputeCircleCollider2World(*body.object, *body.circle, cx, cy, cr);
        out.minX = cx - cr;
        out.maxX = cx + cr;
        out.minY = cy - cr;
        out.maxY = cy + cr;
        return;
    }
    if (body.box != nullptr) {
        ComputeBoxCollider2WorldAabb(*body.object, *body.box, out);
        return;
    }
    out = CollisionAabb2{};
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

[[nodiscard]] bool GetPrimaryColliderFiltersFromDynamic(
        const CircleCollider2DComponent* circ,
        const BoxCollider2DComponent* box,
        bool& outTrigger,
        std::uint16_t& outCat,
        std::uint16_t& outMask) noexcept {
    if (circ != nullptr) {
        outTrigger = circ->GetIsTrigger();
        outCat = circ->GetCategoryBits();
        outMask = circ->GetMaskBits();
        return true;
    }
    if (box != nullptr) {
        outTrigger = box->GetIsTrigger();
        outCat = box->GetCategoryBits();
        outMask = box->GetMaskBits();
        return true;
    }
    return false;
}

[[nodiscard]] bool OverlapWorldCircleWithDynamicQueryBody(
        const float qx,
        const float qy,
        const float qr,
        const DynamicQueryBody2D& b) noexcept {
    if (b.circle != nullptr) {
        float ox = 0.0F;
        float oy = 0.0F;
        float orr = 0.0F;
        ComputeCircleCollider2World(*b.object, *b.circle, ox, oy, orr);
        return CollisionCirclesOverlap(qx, qy, qr, ox, oy, orr);
    }
    if (b.box != nullptr) {
        CollisionAabb2 ba{};
        ComputeBoxCollider2WorldAabb(*b.object, *b.box, ba);
        return CollisionAabb2OverlapsCircle(ba, qx, qy, qr);
    }
    return false;
}

void DynamicQueryBodyCenterAndSlack(const DynamicQueryBody2D& b, float& cx, float& cy, float& slack) noexcept {
    if (b.circle != nullptr) {
        float r = 0.0F;
        ComputeCircleCollider2World(*b.object, *b.circle, cx, cy, r);
        slack = r;
        return;
    }
    CollisionAabb2 ba{};
    ComputeBoxCollider2WorldAabb(*b.object, *b.box, ba);
    cx = 0.5F * (ba.minX + ba.maxX);
    cy = 0.5F * (ba.minY + ba.maxY);
    const float dx = ba.maxX - ba.minX;
    const float dy = ba.maxY - ba.minY;
    slack = 0.5F * std::sqrt(dx * dx + dy * dy);
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
        const StaticCollider2D& st,
        const float ox,
        const float oy,
        const float rdx,
        const float rdy,
        const float cosHalfAngle,
        const float maxReach) noexcept {
    float cx = 0.0F;
    float cy = 0.0F;
    float slack = 0.0F;
    if (st.shape == StaticCollider2DShape::Circle) {
        cx = st.circleCx;
        cy = st.circleCy;
        slack = st.circleR;
    } else {
        cx = 0.5F * (st.aabb.minX + st.aabb.maxX);
        cy = 0.5F * (st.aabb.minY + st.aabb.maxY);
        const float dx = st.aabb.maxX - st.aabb.minX;
        const float dy = st.aabb.maxY - st.aabb.minY;
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

    Array<DynamicQueryBody2D> bodies;
    GatherDynamicQueryBodies2D(world, bodies);
    const std::size_t n = bodies.GetSize();
    if (n == 0) {
        return;
    }

    SpatialHashGrid2D dynBroad;
    dynBroad.Clear();
    const float cell = (cellWorldSize > 1.0e-4F) ? cellWorldSize : 4.0F;
    dynBroad.SetCellSize(cell);

    for (std::size_t i = 0; i < n; ++i) {
        CollisionAabb2 aabb{};
        ComputeDynamicQueryBodyAabb(bodies[i], aabb);
        dynBroad.InsertIndexedAabb(static_cast<std::uint32_t>(i), aabb);
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
        const DynamicQueryBody2D& b = bodies[bi];
        if (ignore != nullptr && b.object == ignore) {
            continue;
        }
        bool trig = false;
        std::uint16_t cat = 1u;
        std::uint16_t mask = 0xFFFFu;
        if (!GetPrimaryColliderFiltersFromDynamic(b.circle, b.box, trig, cat, mask)) {
            continue;
        }
        if (!PassesDynamicQueryFilter(filter, trig, cat, mask)) {
            continue;
        }
        if (!OverlapWorldCircleWithDynamicQueryBody(originX, originY, radius, b)) {
            continue;
        }
        if (useSector) {
            float rcx = 0.0F;
            float rcy = 0.0F;
            float slack = 0.0F;
            DynamicQueryBodyCenterAndSlack(b, rcx, rcy, slack);
            if (!RoughCenterInFiniteSector(rcx, rcy, slack, originX, originY, rdx, rdy, cosHalfAngle, radius)) {
                continue;
            }
        }
        PhysicsQueryHitDynamic2D h{};
        h.owner = b.object;
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
        const StaticBroadPhase2D& broadPhase,
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
    broadPhase.grid.QueryUniquePayloadIndices(query, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t si = candidates[i];
        if (si >= broadPhase.statics.GetSize()) {
            continue;
        }
        const StaticCollider2D& st = broadPhase.statics[si];
        if (!PassesQueryFilter(filter, st)) {
            continue;
        }
        if (!StaticCollider2DOverlapsWorldCircle(st, originX, originY, radius)) {
            continue;
        }
        if (!StaticRoughInArcSector(st, originX, originY, rdx, rdy, cosHalf, radius)) {
            continue;
        }
        PhysicsQueryHit2D h{};
        h.staticColliderIndex = si;
        h.owner = st.owner;
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
    StaticBroadPhase2D bp;
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

}  // namespace Spark
