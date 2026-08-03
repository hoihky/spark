#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

class GameWorld;

/**
 * Owns baked static 3D colliders and a uniform-cell spatial hash (broad-phase).
 * Shared by <c>PhysicsWorld3D</c>, character controllers, and trigger volumes.
 */
class BroadPhase3D {
public:
    /** Default cell size matches <c>PhysicsWorld3D::DefaultBroadPhaseCellSize</c> (2 world units). */
    void Rebuild(GameWorld& world, float cellWorldSize = 2.0F);

    void Clear() noexcept;

    [[nodiscard]] const Array<Collider3D>& GetColliders() const noexcept { return colliders; }
    [[nodiscard]] Array<Collider3D>& GetColliders() noexcept { return colliders; }

    /** Backward-compatible name; returns the same collider array as <c>GetColliders</c>. */
    [[nodiscard]] const Array<Collider3D>& GetStatics() const noexcept { return colliders; }
    [[nodiscard]] Array<Collider3D>& GetStatics() noexcept { return colliders; }

    [[nodiscard]] const SpatialHashGrid3D& GetGrid() const noexcept { return grid; }
    [[nodiscard]] SpatialHashGrid3D& GetGrid() noexcept { return grid; }
    [[nodiscard]] float GetCellSize() const noexcept { return grid.GetCellSize(); }

private:
    Array<Collider3D> colliders{};
    SpatialHashGrid3D grid{};
};

}  // namespace Spark
