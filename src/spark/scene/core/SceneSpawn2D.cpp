#include "spark/scene/core/SceneSpawn2D.hpp"

#include "spark/scene/core/SpawnPoint2DService.hpp"

namespace Spark {

SceneSpawnPose2D FindSpawnPoint2D(const GameWorld& world, const char* spawnName) noexcept {
    static const SpawnPoint2DService kDefaultService{};
    return kDefaultService.Resolve(world, spawnName);
}

}  // namespace Spark
