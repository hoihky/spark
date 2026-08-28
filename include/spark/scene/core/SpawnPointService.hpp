#pragma once

#include "spark/scene/core/SceneSpawn.hpp"

namespace Spark {

class GameWorld;

/** Resolves named <c>SpawnPointComponent</c> poses from a live world (Strategy-style service). */
class SpawnPointService final {
public:
    [[nodiscard]] SceneSpawnPose Resolve(const GameWorld& world, const char* spawnName) const noexcept;
};

}  // namespace Spark
