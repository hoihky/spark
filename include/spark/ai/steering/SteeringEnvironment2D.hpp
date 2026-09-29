#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"

namespace Spark {

/** Shared steering context for 2D behaviors (world XY plane). */
struct SteeringEnvironment2D {
    float maxSteeringSpeed = 8.0F;
    float maxAcceleration = 24.0F;
    float waypointArriveRadius = 0.15F;

    const Array<Vector2>* pathPoints = nullptr;
    int pathIndex = 0;

    const Array<Vector2>* obstacleCenters = nullptr;
    const Array<float>* obstacleRadii = nullptr;
    float obstacleAvoidLookahead = 2.0F;
    float obstacleAvoidSideWeight = 1.0F;
};

}  // namespace Spark
