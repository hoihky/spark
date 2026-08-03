#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

/** Output targets passed to each <c>IColliderBakeStrategy2D</c> during a broad-phase rebuild. */
struct ColliderBakeContext2D {
    Array<Collider2D>& colliders;
    SpatialHashGrid2D& grid;
};

}  // namespace Spark
