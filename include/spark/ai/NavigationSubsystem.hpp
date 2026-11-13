#pragma once

namespace Spark {

class GameWorld;
class GridNavAgent2DComponent;
class Rigidbody2DComponent;
class TransformComponent;

void ProcessNavMeshAgents(GameWorld& world) noexcept;

/** Replans <c>GridNavAgent2DComponent</c> paths from tilemap gameplay grids (XY plane). */
void ProcessGridNavAgents2D(GameWorld& world, float deltaTimeSeconds) noexcept;

/**
 * Sets <c>Rigidbody2D</c> velocity toward the active nav waypoint (call after <c>ProcessGridNavAgents2D</c>).
 * Returns false when the agent has no path.
 */
[[nodiscard]] bool ApplyGridNavAgent2DRigidbodyMotion(
        GridNavAgent2DComponent& agent,
        TransformComponent& transform,
        Rigidbody2DComponent& rigidbody,
        float maxSpeed,
        float arriveRadius,
        float deltaTimeSeconds) noexcept;

/** Path following via <c>SteeringPathFollowing2D</c> (grid nav + 2D steering product path). */
[[nodiscard]] bool ApplyGridNavAgent2DSteeringMotion(
        GridNavAgent2DComponent& agent,
        TransformComponent& transform,
        Rigidbody2DComponent& rigidbody,
        float maxSpeed,
        float arriveRadius,
        float deltaTimeSeconds) noexcept;

/** Navigation services (static façade over the free helpers above). */
class NavSubsystem final {
public:
    static void ProcessNavMesh(GameWorld& world) noexcept { ProcessNavMeshAgents(world); }

    static void ProcessGridNav2D(GameWorld& world, float deltaTimeSeconds) noexcept {
        ProcessGridNavAgents2D(world, deltaTimeSeconds);
    }

    [[nodiscard]] static bool ApplyGridNavRigidbodyMotion(
            GridNavAgent2DComponent& agent,
            TransformComponent& transform,
            Rigidbody2DComponent& rigidbody,
            float maxSpeed,
            float arriveRadius,
            float deltaTimeSeconds) noexcept {
        return ApplyGridNavAgent2DRigidbodyMotion(
                agent, transform, rigidbody, maxSpeed, arriveRadius, deltaTimeSeconds);
    }

    [[nodiscard]] static bool ApplyGridNavSteeringMotion(
            GridNavAgent2DComponent& agent,
            TransformComponent& transform,
            Rigidbody2DComponent& rigidbody,
            float maxSpeed,
            float arriveRadius,
            float deltaTimeSeconds) noexcept {
        return ApplyGridNavAgent2DSteeringMotion(
                agent, transform, rigidbody, maxSpeed, arriveRadius, deltaTimeSeconds);
    }
};

}  // namespace Spark
