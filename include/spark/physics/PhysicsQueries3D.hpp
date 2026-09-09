#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/BroadPhase3D.hpp"
#include "spark/physics/CollisionFilter2D.hpp"

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
    float distanceAlongRay = 0.0F;
    Vector3 hitPoint{Vector3::Zero};
    Vector3 hitNormal{Vector3::UnitY};
    std::uint32_t staticColliderIndex = 0;
    GameObject* owner = nullptr;
};

/**
 * Static 3D physics query service (broad-phase rebuild + raycasts).
 */
class PhysicsQueryWorld3D {
public:
    explicit PhysicsQueryWorld3D(float cellWorldSizeIn = 2.0F) : cellWorldSize(cellWorldSizeIn) {}

    void RebuildStatics(GameWorld& world);

    [[nodiscard]] const BroadPhase3D& GetBroadPhase() const noexcept { return broadPhase; }
    [[nodiscard]] BroadPhase3D& GetBroadPhase() noexcept { return broadPhase; }

    void SetCellWorldSize(float cellWorldSizeIn) noexcept { cellWorldSize = cellWorldSizeIn; }
    [[nodiscard]] float GetCellWorldSize() const noexcept { return cellWorldSize; }

    [[nodiscard]] bool RaycastStatics(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            const PhysicsQueryFilter3D& filter,
            PhysicsRaycastHit3D& outHit) const;

private:
    BroadPhase3D broadPhase{};
    float cellWorldSize = 2.0F;
};

}  // namespace Spark
