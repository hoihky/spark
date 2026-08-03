#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

/** Appends a collider to the broad-phase array and inserts its bounds into the spatial hash. */
inline void PushCollider2D(
        Array<Collider2D>& colliders,
        SpatialHashGrid2D& grid,
        Collider2D&& collider) {
    const std::uint32_t idx = static_cast<std::uint32_t>(colliders.GetSize());
    const CollisionAabb2 bounds = collider.GetBounds();
    colliders.PushBack(MoveTemp(collider));
    grid.InsertIndexedAabb(idx, bounds);
}

}  // namespace Spark
