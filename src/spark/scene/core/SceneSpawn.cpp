#include "spark/scene/core/SceneSpawn.hpp"

#include "spark/scene/core/SpawnPointService.hpp"

namespace Spark {

SceneSpawnPose FindSpawnPoint(const GameWorld& world, const char* spawnName) noexcept {
    static const SpawnPointService kDefaultService{};
    return kDefaultService.Resolve(world, spawnName);
}

}  // namespace Spark
