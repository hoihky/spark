#include "spark/physics/PhysicsQueries3D.hpp"

#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/physics/shapes/IShape3D.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

namespace {

[[nodiscard]] bool PassesQueryFilter(const PhysicsQueryFilter3D& filter, const Collider3D& collider) noexcept {
    return collider.GetFilter().PassesQueryFilter(
            filter.queryCategoryBits, filter.queryMaskBits, filter.hitSolids, filter.hitTriggers);
}

[[nodiscard]] Vector3 ComputeBoxHitNormal(const CollisionAabb3& aabb, const Vector3& hitPoint) noexcept {
    const float dxMin = std::fabs(hitPoint.x - aabb.minX);
    const float dxMax = std::fabs(hitPoint.x - aabb.maxX);
    const float dyMin = std::fabs(hitPoint.y - aabb.minY);
    const float dyMax = std::fabs(hitPoint.y - aabb.maxY);
    const float dzMin = std::fabs(hitPoint.z - aabb.minZ);
    const float dzMax = std::fabs(hitPoint.z - aabb.maxZ);
    float best = dxMin;
    Vector3 normal{-1.0F, 0.0F, 0.0F};
    if (dxMax < best) {
        best = dxMax;
        normal = {1.0F, 0.0F, 0.0F};
    }
    if (dyMin < best) {
        best = dyMin;
        normal = {0.0F, -1.0F, 0.0F};
    }
    if (dyMax < best) {
        best = dyMax;
        normal = {0.0F, 1.0F, 0.0F};
    }
    if (dzMin < best) {
        best = dzMin;
        normal = {0.0F, 0.0F, -1.0F};
    }
    if (dzMax < best) {
        normal = {0.0F, 0.0F, 1.0F};
    }
    return normal;
}

}  // namespace

void PhysicsQueryWorld3D::RebuildStatics(GameWorld& world) {
    broadPhase.Rebuild(world, cellWorldSize);
}

void PhysicsQueryWorld3D::RebuildDynamics(GameWorld& world) {
    dynamicCatalog.Rebuild(world);
}

void PhysicsQueryWorld3D::RebuildAll(GameWorld& world) {
    RebuildStatics(world);
    RebuildDynamics(world);
}

void PhysicsQueryWorld3D::OverlapSphereStatics(
        const Vector3& center,
        const float radius,
        const PhysicsQueryFilter3D& filter,
        Array<PhysicsQueryHit3D>& outHits) const {
    outHits.Clear();
    if (radius < 0.0F) {
        return;
    }

    CollisionAabb3 query{};
    query.minX = center.x - radius;
    query.maxX = center.x + radius;
    query.minY = center.y - radius;
    query.maxY = center.y + radius;
    query.minZ = center.z - radius;
    query.maxZ = center.z + radius;

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(query, candidates);

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t staticIndex = candidates[i];
        if (staticIndex >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider3D& collider = broadPhase.GetColliders()[staticIndex];
        if (!PassesQueryFilter(filter, collider)) {
            continue;
        }
        if (!collider.OverlapsSphere(center, radius)) {
            continue;
        }
        PhysicsQueryHit3D hit{};
        hit.staticColliderIndex = staticIndex;
        hit.owner = collider.GetOwner();
        outHits.PushBack(hit);
    }
}

void PhysicsQueryWorld3D::OverlapSphereDynamics(
        const Vector3& center,
        const float radius,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic3D>& outHits) const {
    dynamicCatalog.OverlapSphere(center, radius, filter, ignore, outHits);
}

void PhysicsQueryWorld3D::OverlapSphere(
        GameWorld& world,
        const Vector3& center,
        const float radius,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHit3D>& outStaticHits,
        Array<PhysicsQueryHitDynamic3D>& outDynamicHits) {
    RebuildAll(world);
    OverlapSphereStatics(center, radius, filter, outStaticHits);
    OverlapSphereDynamics(center, radius, filter, ignore, outDynamicHits);
}

bool PhysicsQueryWorld3D::RaycastStatics(
        const Vector3& origin,
        const Vector3& direction,
        const float maxDistance,
        const PhysicsQueryFilter3D& filter,
        PhysicsRaycastHit3D& outHit) const {
    if (maxDistance <= 0.0F) {
        return false;
    }

    const Vector3 dir = direction.LengthSquared() > 1.0e-8F ? direction.Normalized() : Vector3{0.0F, -1.0F, 0.0F};
    const Vector3 end = origin + dir * maxDistance;
    CollisionAabb3 sweep{};
    sweep.minX = (std::min)(origin.x, end.x);
    sweep.maxX = (std::max)(origin.x, end.x);
    sweep.minY = (std::min)(origin.y, end.y);
    sweep.maxY = (std::max)(origin.y, end.y);
    sweep.minZ = (std::min)(origin.z, end.z);
    sweep.maxZ = (std::max)(origin.z, end.z);
    constexpr float kPad = 1.0e-3F;
    sweep.minX -= kPad;
    sweep.maxX += kPad;
    sweep.minY -= kPad;
    sweep.maxY += kPad;
    sweep.minZ -= kPad;
    sweep.maxZ += kPad;

    Array<std::uint32_t> candidates;
    broadPhase.GetGrid().QueryUniquePayloadIndices(sweep, candidates);

    float bestDistance = std::numeric_limits<float>::infinity();
    bool any = false;
    PhysicsRaycastHit3D best{};

    for (std::size_t i = 0; i < candidates.GetSize(); ++i) {
        const std::uint32_t staticIndex = candidates[i];
        if (staticIndex >= broadPhase.GetColliders().GetSize()) {
            continue;
        }
        const Collider3D& collider = broadPhase.GetColliders()[staticIndex];
        if (!PassesQueryFilter(filter, collider)) {
            continue;
        }
        Ray3D ray{};
        ray.origin = origin;
        ray.direction = dir;
        ray.maxDistance = maxDistance;
        float distance = 0.0F;
        if (!collider.GetShape().Raycast(ray, distance)) {
            continue;
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            best.distanceAlongRay = distance;
            best.hitPoint = origin + dir * distance;
            best.staticColliderIndex = staticIndex;
            best.owner = collider.GetOwner();
            best.isDynamicHit = false;
            const CollisionAabb3 bounds = collider.GetBounds();
            best.hitNormal = ComputeBoxHitNormal(bounds, best.hitPoint);
            any = true;
        }
    }

    if (!any) {
        return false;
    }
    outHit = best;
    return true;
}

bool PhysicsQueryWorld3D::RaycastDynamics(
        const Vector3& origin,
        const Vector3& direction,
        const float maxDistance,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        PhysicsRaycastHit3D& outHit) const {
    return dynamicCatalog.Raycast(origin, direction, maxDistance, filter, ignore, outHit);
}

bool PhysicsQueryWorld3D::Raycast(
        GameWorld& world,
        const Vector3& origin,
        const Vector3& direction,
        const float maxDistance,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        PhysicsRaycastHit3D& outHit) {
    RebuildAll(world);

    PhysicsRaycastHit3D staticHit{};
    PhysicsRaycastHit3D dynamicHit{};
    const bool hasStatic = RaycastStatics(origin, direction, maxDistance, filter, staticHit);
    const bool hasDynamic = RaycastDynamics(origin, direction, maxDistance, filter, ignore, dynamicHit);

    if (!hasStatic && !hasDynamic) {
        return false;
    }
    if (hasStatic && (!hasDynamic || staticHit.distanceAlongRay <= dynamicHit.distanceAlongRay)) {
        outHit = staticHit;
        return true;
    }
    outHit = dynamicHit;
    return true;
}

}  // namespace Spark
