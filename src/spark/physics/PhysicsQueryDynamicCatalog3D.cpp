#include "spark/physics/PhysicsQueryDynamicCatalog3D.hpp"

#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsAttachedBody3DComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/physics/PhysicsQueries3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/physics/colliders/PhysicsCollider3DFilter.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

namespace {

void BuildCharacterControllerCollider(
        GameObject& owner,
        const CharacterController3DComponent& controller,
        DynamicCollider3D& outCollider) noexcept {
    DynamicCollider3DSim snapshot{};
    const Matrix4 worldMatrix = owner.GetWorldMatrix();
    const Vector3 localCenter = controller.GetCenterOffset();
    const Vector4 pc = worldMatrix * Vector4(localCenter.x, localCenter.y, localCenter.z, 1.0F);
    const float w = (std::fabs(pc.w) < 1.0e-8F) ? 1.0F : pc.w;
    snapshot.shape = DynamicCollider3DShape::Sphere;
    snapshot.sphereCenter = {pc.x / w, pc.y / w, pc.z / w};
    const float sx = std::sqrt(
            worldMatrix.m[0] * worldMatrix.m[0] + worldMatrix.m[1] * worldMatrix.m[1] +
            worldMatrix.m[2] * worldMatrix.m[2]);
    const float sy = std::sqrt(
            worldMatrix.m[4] * worldMatrix.m[4] + worldMatrix.m[5] * worldMatrix.m[5] +
            worldMatrix.m[6] * worldMatrix.m[6]);
    const float sz = std::sqrt(
            worldMatrix.m[8] * worldMatrix.m[8] + worldMatrix.m[9] * worldMatrix.m[9] +
            worldMatrix.m[10] * worldMatrix.m[10]);
    const float scale = std::max({sx, sy, sz});
    snapshot.sphereRadius = controller.GetRadius() * scale;
    RebuildDynamicCollider3DBounds(snapshot);
    outCollider = DynamicCollider3D::FromLegacySnapshot(snapshot);
}

[[nodiscard]] ColliderFilter BuildCharacterControllerFilter(const CharacterController3DComponent& controller) noexcept {
    ColliderFilter filter{};
    filter.categoryBits = controller.GetCategoryBits();
    filter.maskBits = controller.GetMaskBits();
    filter.isTrigger = false;
    return filter;
}

[[nodiscard]] Vector3 RayHitNormalTowardCollider(const DynamicCollider3D& collider, const Vector3& hitPoint) noexcept {
    const Vector3 center = collider.GetTrackingPoint();
    Vector3 n{hitPoint.x - center.x, hitPoint.y - center.y, hitPoint.z - center.z};
    const float len2 = n.x * n.x + n.y * n.y + n.z * n.z;
    if (len2 < 1.0e-10F) {
        return Vector3{0.0F, 1.0F, 0.0F};
    }
    return n.Normalized();
}

}  // namespace

bool PhysicsQueryDynamicCatalog3D::PassesFilter(const Entry& entry, const PhysicsQueryFilter3D& filter) const noexcept {
    return entry.filter.PassesQueryFilter(
            filter.queryCategoryBits, filter.queryMaskBits, filter.hitSolids, filter.hitTriggers);
}

void PhysicsQueryDynamicCatalog3D::Rebuild(GameWorld& world) {
    entries.Clear();
    broadPhase.Clear();
    const float cell = (cellWorldSize > 1.0e-4F) ? cellWorldSize : 2.0F;
    broadPhase.SetCellSize(cell);

    Array<DynamicBody3D> rigidbodies;
    CollectDynamicBodies3D(world, rigidbodies);
    for (std::size_t i = 0; i < rigidbodies.GetSize(); ++i) {
        DynamicBody3D& body = rigidbodies[i];
        if (body.obj == nullptr) {
            continue;
        }
        if (const auto* attachment = body.obj->GetComponent<PhysicsAttachedBody3DComponent>();
            attachment != nullptr && attachment->IsAttached()) {
            continue;
        }
        RefreshDynamicBody3D(body);
        Entry entry{};
        entry.owner = body.obj;
        entry.collider = DynamicCollider3D::FromLegacySnapshot(body.collider.ToLegacySnapshot());
        entry.filter = PhysicsCollider3DFilter::Resolve(*body.obj);
        const CollisionAabb3 bounds = entry.collider.GetBounds();
        entries.PushBack(MoveTemp(entry));
        broadPhase.InsertIndexedAabb(static_cast<std::uint32_t>(entries.GetSize() - 1), bounds);
    }

    if (!includeCharacterControllers) {
        return;
    }

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        auto* controller = object->GetComponent<CharacterController3DComponent>();
        if (controller == nullptr) {
            return;
        }
        Entry entry{};
        entry.owner = object;
        entry.filter = BuildCharacterControllerFilter(*controller);
        BuildCharacterControllerCollider(*object, *controller, entry.collider);
        const CollisionAabb3 bounds = entry.collider.GetBounds();
        entries.PushBack(MoveTemp(entry));
        broadPhase.InsertIndexedAabb(static_cast<std::uint32_t>(entries.GetSize() - 1), bounds);
    });
}

void PhysicsQueryDynamicCatalog3D::OverlapSphere(
        const Vector3& center,
        const float radius,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        Array<PhysicsQueryHitDynamic3D>& outHits) const {
    outHits.Clear();
    if (radius < 0.0F || entries.GetSize() == 0) {
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
    broadPhase.QueryUniquePayloadIndices(query, candidates);

    for (std::size_t k = 0; k < candidates.GetSize(); ++k) {
        const std::uint32_t index = candidates[k];
        if (index >= entries.GetSize()) {
            continue;
        }
        const Entry& entry = entries[index];
        if (ignore != nullptr && entry.owner == ignore) {
            continue;
        }
        if (!PassesFilter(entry, filter)) {
            continue;
        }
        if (!entry.collider.GetShape().OverlapsSphere(center, radius)) {
            continue;
        }
        PhysicsQueryHitDynamic3D hit{};
        hit.owner = entry.owner;
        outHits.PushBack(hit);
    }
}

bool PhysicsQueryDynamicCatalog3D::Raycast(
        const Vector3& origin,
        const Vector3& direction,
        const float maxDistance,
        const PhysicsQueryFilter3D& filter,
        GameObject* ignore,
        PhysicsRaycastHit3D& outHit) const {
    if (maxDistance <= 0.0F || entries.GetSize() == 0) {
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
    broadPhase.QueryUniquePayloadIndices(sweep, candidates);

    float bestDistance = std::numeric_limits<float>::infinity();
    bool any = false;
    PhysicsRaycastHit3D best{};

    for (std::size_t k = 0; k < candidates.GetSize(); ++k) {
        const std::uint32_t index = candidates[k];
        if (index >= entries.GetSize()) {
            continue;
        }
        const Entry& entry = entries[index];
        if (ignore != nullptr && entry.owner == ignore) {
            continue;
        }
        if (!PassesFilter(entry, filter)) {
            continue;
        }
        Ray3D ray{};
        ray.origin = origin;
        ray.direction = dir;
        ray.maxDistance = maxDistance;
        float distance = 0.0F;
        if (!entry.collider.GetShape().Raycast(ray, distance)) {
            continue;
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            best.distanceAlongRay = distance;
            best.hitPoint = origin + dir * distance;
            best.hitNormal = RayHitNormalTowardCollider(entry.collider, best.hitPoint);
            best.owner = entry.owner;
            best.staticColliderIndex = PhysicsRaycastHit3D::kNoStaticColliderIndex;
            best.isDynamicHit = true;
            any = true;
        }
    }

    if (!any) {
        return false;
    }
    outHit = best;
    return true;
}

}  // namespace Spark
