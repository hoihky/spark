#pragma once

namespace Spark {

class GameWorld;

void ProcessNavMeshAgents(GameWorld& world) noexcept;

/** Replans <c>GridNavAgent2DComponent</c> paths from tilemap gameplay grids (XY plane). */
void ProcessGridNavAgents2D(GameWorld& world, float deltaTimeSeconds) noexcept;

}  // namespace Spark
