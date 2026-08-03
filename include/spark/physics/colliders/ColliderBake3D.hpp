#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

inline void PushCollider3D(
        Array<Collider3D>& colliders,
        SpatialHashGrid3D& grid,
        Collider3D&& collider) {
    const std::uint32_t idx = static_cast<std::uint32_t>(colliders.GetSize());
    const CollisionAabb3 bounds = collider.GetBounds();
    colliders.PushBack(MoveTemp(collider));
    grid.InsertIndexedAabb(idx, bounds);
}

}  // namespace Spark
