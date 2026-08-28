#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/core/SceneSpawn.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SpawnPointService.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/** Result of loading a designed level from disk. */
struct SceneLevelLoadResult {
    bool success = false;
    SceneInstanceId instanceId = kInvalidSceneInstanceId;
    GameObject* primaryEntity = nullptr;
    SceneSpawnPose spawnPose{};
    Utf8String message{};
};

using ScenePrimaryEntitySelector = GameObject* (*)(GameWorld& world, SceneInstanceId instanceId);

/**
 * Template Method for loading a <c>.sparkscene</c> level and locating a primary entity
 * via a caller-supplied selector plus optional spawn pose.
 */
class SceneLevelLoader final {
public:
    explicit SceneLevelLoader(SceneLoadSession& session) noexcept : loadSession(session) {}

    [[nodiscard]] SceneLevelLoadResult Load(
            GameWorld& world,
            const char* scenePath,
            ScenePrimaryEntitySelector selectPrimary,
            const char* spawnName = "Player") noexcept;

private:
    SceneLoadSession& loadSession;
    SpawnPointService spawnPoints{};
};

}  // namespace Spark
