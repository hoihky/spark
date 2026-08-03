#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

/** Output targets passed to each <c>IColliderBakeStrategy3D</c> during a broad-phase rebuild. */
struct ColliderBakeContext3D {
    Array<Collider3D>& colliders;
    SpatialHashGrid3D& grid;
};

}  // namespace Spark
