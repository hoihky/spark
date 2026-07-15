#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Per-frame context for 3D steering: targets, bounds, optional obstacles / path / flock neighbors.
 * Populated by gameplay or demos; behaviors read only what they need (Interface segregation).
 */
struct SteeringEnvironment3D {
    Vector3 targetPosition{Vector3::Zero};
    Vector3 targetVelocity{Vector3::Zero};
    float arriveSlowRadius = 4.0F;

    Vector3 secondaryPosition{Vector3::Zero};
    Vector3 secondaryVelocity{Vector3::Zero};

    Vector3 pursuerPosition{Vector3::Zero};
    Vector3 pursuerVelocity{Vector3::Zero};

    Vector3 worldBoundsMin{-40.0F, 0.0F, -40.0F};
    Vector3 worldBoundsMax{40.0F, 24.0F, 40.0F};
    float wallAvoidMargin = 2.0F;

    float obstacleAvoidLookahead = 3.0F;
    float obstacleAvoidSideWeight = 2.2F;
    float hideAgentRadius = 0.45F;
    const Array<Vector3>* obstacleCenters = nullptr;
    const Array<float>* obstacleRadii = nullptr;

    const Array<Vector3>* pathPoints = nullptr;
    int pathIndex = 0;
    float pathWaypointRadius = 1.0F;
    bool pathLoop = true;

    Vector3 leaderPosition{Vector3::Zero};
    Vector3 leaderVelocity{Vector3::Zero};
    /** Offset in a local frame: x = right of leader, y = up, z = forward along leader velocity (XZ dominant). */
    Vector3 offsetPursuitLocal{-2.0F, 0.0F, -1.5F};

    const Array<Vector3>* flockPositions = nullptr;
    const Array<Vector3>* flockVelocities = nullptr;
    std::size_t flockSelfIndex = 0;
    float separationRadius = 3.5F;
    float cohesionRadius = 10.0F;

    /** Cruise speed used when building desired velocity (Seek/Flee/Pursuit, etc.). */
    float maxSteeringSpeed = 9.0F;
    float maxAcceleration = 22.0F;
};

}  // namespace Spark
