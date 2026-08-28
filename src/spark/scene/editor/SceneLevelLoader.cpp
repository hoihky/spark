#include "spark/scene/editor/SceneLevelLoader.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

SceneLevelLoadResult SceneLevelLoader::Load(
        GameWorld& world,
        const char* scenePath,
        const ScenePrimaryEntitySelector selectPrimary,
        const char* spawnName) noexcept {
    SceneLevelLoadResult result{};
    if (scenePath == nullptr || scenePath[0] == '\0') {
        result.message = Utf8String("Scene path is empty.");
        return result;
    }
    if (!ScenePathResolver::FileExists(scenePath)) {
        result.message = Utf8String("Scene file not found.");
        return result;
    }
    if (selectPrimary == nullptr) {
        result.message = Utf8String("Primary entity selector is required.");
        return result;
    }

    SceneLoadOptions options{};
    options.assetsRoot = ScenePathResolver::AssetsRoot();
    options.additive = true;
    result.instanceId = loadSession.LoadBlocking(scenePath, options);
    if (result.instanceId == kInvalidSceneInstanceId) {
        result.message = Utf8String("Scene load failed.");
        return result;
    }

    result.primaryEntity = selectPrimary(world, result.instanceId);
    if (result.primaryEntity == nullptr) {
        result.message = Utf8String("Scene loaded but primary entity was not found.");
        return result;
    }

    result.spawnPose = spawnPoints.Resolve(world, spawnName);
    result.success = true;
    result.message = Utf8String("Scene loaded.");
    return result;
}

}  // namespace Spark
