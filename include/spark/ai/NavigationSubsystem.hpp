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

}  // namespace Spark
