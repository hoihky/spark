#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * 2D ray segment: <c>point(t) = origin + t * direction</c> for <c>t in [0, maxDistance]</c>.
 * Direction need not be unit length; distance along the ray uses the same parameterization as
 * <c>RaycastSegmentAabb2</c> / <c>RaycastSegmentCircle2</c>.
 */
struct Ray2D {
    Vector2 origin{Vector2::Zero};
    Vector2 direction{Vector2::UnitX};
    float maxDistance = 0.0F;
};

/**
 * 3D ray segment: <c>point(t) = origin + t * direction</c> for <c>t in [0, maxDistance]</c>.
 */
struct Ray3D {
    Vector3 origin{Vector3::Zero};
    Vector3 direction{Vector3::UnitX};
    float maxDistance = 0.0F;
};

}  // namespace Spark
