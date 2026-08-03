#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

class GameWorld;

/** Default uniform-cell size used when rebuilding the static broad-phase (matches 2D simulation). */
inline constexpr float kBroadPhase2DDefaultCellSize = 4.0F;

/**
 * Owns baked static 2D colliders and a uniform-cell spatial hash (broad-phase).
 * Rebuild once per frame, then share across simulation and queries (Single Responsibility).
 */
class BroadPhase2D {
public:
    /** Default cell size matches <c>PhysicsWorld2D::DefaultBroadPhaseCellSize</c> (4 world units). */
    void Rebuild(GameWorld& world, float cellWorldSize = 4.0F);

    void Clear() noexcept;

    [[nodiscard]] const Array<Collider2D>& GetColliders() const noexcept { return colliders; }
    [[nodiscard]] Array<Collider2D>& GetColliders() noexcept { return colliders; }

    /** Backward-compatible name; returns the same collider array as <c>GetColliders</c>. */
    [[nodiscard]] const Array<Collider2D>& GetStatics() const noexcept { return colliders; }
    [[nodiscard]] Array<Collider2D>& GetStatics() noexcept { return colliders; }

    [[nodiscard]] const SpatialHashGrid2D& GetGrid() const noexcept { return grid; }
    [[nodiscard]] SpatialHashGrid2D& GetGrid() noexcept { return grid; }
    [[nodiscard]] float GetCellSize() const noexcept { return grid.GetCellSize(); }

private:
    Array<Collider2D> colliders{};
    SpatialHashGrid2D grid{};
};

/** @deprecated Prefer <c>BroadPhase2D</c>. */
using StaticBroadPhase2D [[deprecated("Use BroadPhase2D")]] = BroadPhase2D;

}  // namespace Spark
