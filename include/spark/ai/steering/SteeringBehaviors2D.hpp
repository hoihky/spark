#pragma once

#include "spark/ai/steering/SteeringEnvironment2D.hpp"

namespace Spark {

/** Steer toward the active waypoint on <c>env.pathPoints</c> (see <c>SteeringEnvironment2D</c>). */
class SteeringPathFollowing2D final {
public:
    explicit SteeringPathFollowing2D(const float weight) noexcept : weight(weight) {}

    [[nodiscard]] Vector2 ComputeAcceleration(
            const Vector2& positionXY,
            const Vector2& velocityXY,
            const SteeringEnvironment2D& env) const noexcept;

private:
    float weight = 1.0F;
};

/** Lateral push away from circular obstacles in <c>env</c>. */
class SteeringObstacleAvoidance2D final {
public:
    explicit SteeringObstacleAvoidance2D(const float weight) noexcept : weight(weight) {}

    [[nodiscard]] Vector2 ComputeAcceleration(
            const Vector2& positionXY,
            const Vector2& velocityXY,
            const SteeringEnvironment2D& env) const noexcept;

private:
    float weight = 1.0F;
};

}  // namespace Spark
