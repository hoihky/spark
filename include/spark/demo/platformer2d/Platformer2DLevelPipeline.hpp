#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneSpawn.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneLoadSession;

/** Outcome of loading the authored Kenney <c>sampleMap.tmx</c> level scene. */
struct Platformer2DLevelLoadResult {
    bool success = false;
    Utf8String message{};
    SceneInstanceId instanceId = kInvalidSceneInstanceId;
    GameObject* levelRoot = nullptr;
    SceneSpawnPose playerSpawnPose{};
};

/**
 * Loads <c>platformer_level.sparkscene</c> (TMX map source only) and prepares tile collision + object spawn layers.
 * Object markers are seeded until a companion <c>.sparkmap</c> ships (see product-path doc).
 */
class Platformer2DLevelPipeline final {
public:
    [[nodiscard]] static Platformer2DLevelLoadResult TryLoad(
            GameWorld& world,
            SceneLoadSession& session,
            DemoRootCollection& roots,
            const char* scenePath) noexcept;

    /**
     * Legacy platformer spawn table projected onto the map grid (platformer demo coordinates).
     * Product-path demos should place markers on walkable cells instead.
     */
    static void SeedLegacyPlatformerMarkers(GameObject& levelRoot) noexcept;

    static void EnsureGameplayAttachments(GameObject& levelRoot) noexcept;
};

}  // namespace Spark
