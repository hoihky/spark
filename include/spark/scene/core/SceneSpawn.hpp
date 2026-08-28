#pragma once

#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SpawnPointComponent;

/** Resolved spawn pose from a <c>SpawnPointComponent</c> in the loaded world. */
struct SceneSpawnPose {
    bool found = false;
    Vector3 position{Vector3::Zero};
    Quaternion rotation = Quaternion::Identity;
    GameObject* object = nullptr;
    const SpawnPointComponent* component = nullptr;
};

/** Finds the first enabled spawn point with a matching name (case-sensitive). */
[[nodiscard]] SceneSpawnPose FindSpawnPoint(const GameWorld& world, const char* spawnName) noexcept;

}  // namespace Spark
