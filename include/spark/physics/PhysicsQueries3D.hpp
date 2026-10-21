#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/BroadPhase3D.hpp"
#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/physics/PhysicsQueryDynamicCatalog3D.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Layer filtering for 3D queries (same bitmask rule as 2D).
 */
class PhysicsQueryFilter3D {
public:
    std::uint16_t queryCategoryBits = CollisionFilter2D::DefaultCategory();
    std::uint16_t queryMaskBits = CollisionFilter2D::AllLayersMask();
    bool hitSolids = true;
    bool hitTriggers = true;
};

class PhysicsRaycastHit3D {
public:
    static constexpr std::uint32_t kNoStaticColliderIndex = 0xFFFFFFFFu;

    float distanceAlongRay = 0.0F;
    Vector3 hitPoint{Vector3::Zero};
    Vector3 hitNormal{Vector3::UnitY};
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
    bool isDynamicHit = false;
};

struct PhysicsQueryHit3D {
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
};

/**
 * 3D physics query service: static broad-phase plus a rebuildable dynamic catalog for moving bodies.
 */
class PhysicsQueryWorld3D {
public:
    explicit PhysicsQueryWorld3D(float cellWorldSizeIn = 2.0F)
            : dynamicCatalog(cellWorldSizeIn), cellWorldSize(cellWorldSizeIn) {}

    void RebuildStatics(GameWorld& world);
    void RebuildDynamics(GameWorld& world);
    void RebuildAll(GameWorld& world);

    [[nodiscard]] const BroadPhase3D& GetBroadPhase() const noexcept { return broadPhase; }
    [[nodiscard]] BroadPhase3D& GetBroadPhase() noexcept { return broadPhase; }

    [[nodiscard]] PhysicsQueryDynamicCatalog3D& GetDynamicCatalog() noexcept { return dynamicCatalog; }
    [[nodiscard]] const PhysicsQueryDynamicCatalog3D& GetDynamicCatalog() const noexcept { return dynamicCatalog; }

    void SetCellWorldSize(float cellWorldSizeIn) noexcept {
        cellWorldSize = cellWorldSizeIn;
        dynamicCatalog.SetCellWorldSize(cellWorldSizeIn);
    }
    [[nodiscard]] float GetCellWorldSize() const noexcept { return cellWorldSize; }

    void OverlapSphereStatics(
            const Vector3& center,
            float radius,
            const PhysicsQueryFilter3D& filter,
            Array<PhysicsQueryHit3D>& outHits) const;

    void OverlapSphereDynamics(
            const Vector3& center,
            float radius,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            Array<PhysicsQueryHitDynamic3D>& outHits) const;

    void OverlapSphere(
            GameWorld& world,
            const Vector3& center,
            float radius,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            Array<PhysicsQueryHit3D>& outStaticHits,
            Array<PhysicsQueryHitDynamic3D>& outDynamicHits);

    [[nodiscard]] bool RaycastStatics(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            const PhysicsQueryFilter3D& filter,
            PhysicsRaycastHit3D& outHit) const;

    [[nodiscard]] bool RaycastDynamics(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            PhysicsRaycastHit3D& outHit) const;

    /** Closest hit among static and dynamic targets (rebuilds dynamics from <c>world</c>). */
    [[nodiscard]] bool Raycast(
            GameWorld& world,
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            PhysicsRaycastHit3D& outHit);

private:
    BroadPhase3D broadPhase{};
    PhysicsQueryDynamicCatalog3D dynamicCatalog{};
    float cellWorldSize = 2.0F;
};

}  // namespace Spark
