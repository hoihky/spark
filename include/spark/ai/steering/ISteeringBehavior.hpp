#pragma once

#include "spark/math/Vector2.hpp"

namespace Spark {

class AiBlackboard;

/**
 * Produces a desired acceleration contribution in the XZ plane (x = world X, y = world Z).
 * Interface segregation: each behavior exposes only Compute; composition happens elsewhere.
 */
class ISteeringBehavior {
public:
    virtual ~ISteeringBehavior() = default;
    virtual Vector2 ComputeAcceleration(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const = 0;
};

}  // namespace Spark
