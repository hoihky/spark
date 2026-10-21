#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"
#include "spark/physics/core/ColliderFilter.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class PhysicsQueryFilter3D;
class PhysicsRaycastHit3D;

/** Hit from a dynamic rigidbody or character-controller query (no baked static collider index). */
struct PhysicsQueryHitDynamic3D {
    GameObject* owner = nullptr;
};

/**
 * Rebuildable catalog of queryable 3D dynamics (simulated rigidbodies + optional character controllers).
 * Used by <c>PhysicsQueryWorld3D</c> for overlap and raycast against moving objects.
 */
class PhysicsQueryDynamicCatalog3D {
public:
    explicit PhysicsQueryDynamicCatalog3D(float cellWorldSizeIn = 2.0F) : cellWorldSize(cellWorldSizeIn) {}

    void SetCellWorldSize(float cellWorldSizeIn) noexcept { cellWorldSize = cellWorldSizeIn; }
    [[nodiscard]] float GetCellWorldSize() const noexcept { return cellWorldSize; }

    void SetIncludeCharacterControllers(bool include) noexcept { includeCharacterControllers = include; }
    [[nodiscard]] bool GetIncludeCharacterControllers() const noexcept { return includeCharacterControllers; }

    void Rebuild(GameWorld& world);

    void OverlapSphere(
            const Vector3& center,
            float radius,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            Array<PhysicsQueryHitDynamic3D>& outHits) const;

    [[nodiscard]] bool Raycast(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            const PhysicsQueryFilter3D& filter,
            GameObject* ignore,
            PhysicsRaycastHit3D& outHit) const;

private:
    struct Entry {
        GameObject* owner = nullptr;
        DynamicCollider3D collider{};
        ColliderFilter filter{};
    };

    [[nodiscard]] bool PassesFilter(const Entry& entry, const PhysicsQueryFilter3D& filter) const noexcept;

    Array<Entry> entries{};
    SpatialHashGrid3D broadPhase{};
    float cellWorldSize = 2.0F;
    bool includeCharacterControllers = true;
};

}  // namespace Spark
