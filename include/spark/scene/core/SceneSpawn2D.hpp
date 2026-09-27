#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SpawnPoint2DComponent;

/** Resolved 2D spawn pose from a <c>SpawnPoint2DComponent</c>. */
struct SceneSpawnPose2D {
    bool found = false;
    Vector3 position{Vector3::Zero};
    float facingRadians = 0.0F;
    int teamId = 0;
    GameObject* object = nullptr;
    const SpawnPoint2DComponent* component = nullptr;
};

/** Finds the first active spawn point with a matching name (case-sensitive). */
[[nodiscard]] SceneSpawnPose2D FindSpawnPoint2D(const GameWorld& world, const char* spawnName) noexcept;

}  // namespace Spark
