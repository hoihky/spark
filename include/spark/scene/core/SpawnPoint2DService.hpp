#pragma once

#include "spark/scene/core/SceneSpawn2D.hpp"

namespace Spark {

class GameWorld;

/** Resolves <c>SpawnPoint2DComponent</c> poses from a live world (Strategy-style service). */
class SpawnPoint2DService final {
public:
    [[nodiscard]] SceneSpawnPose2D Resolve(const GameWorld& world, const char* spawnName) const noexcept;
};

}  // namespace Spark
