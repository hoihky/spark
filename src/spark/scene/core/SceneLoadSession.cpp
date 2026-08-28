#include "spark/scene/core/SceneLoadSession.hpp"

namespace Spark {

SceneInstanceId SceneLoadSession::LoadBlocking(const char* path, const SceneLoadOptions& options) {
    return manager.LoadSceneFromFile(path, options);
}

SceneInstanceId SceneLoadSession::BeginAsync(const char* path, const SceneLoadOptions& options) {
    return manager.BeginLoadSceneAsync(path, options);
}

SceneInstanceId SceneLoadSession::BeginAsync(
        const SceneDocument& document,
        const char* path,
        const SceneLoadOptions& options) {
    return manager.BeginLoadSceneAsync(document, path, options);
}

bool SceneLoadSession::AwaitReady(const SceneInstanceId instanceId, const int maxPumpIterations) noexcept {
    if (instanceId == kInvalidSceneInstanceId) {
        return false;
    }
    for (int i = 0; i < maxPumpIterations; ++i) {
        Pump();
        if (manager.IsSceneReady(instanceId)) {
            return true;
        }
        if (manager.HasSceneFailed(instanceId)) {
            return false;
        }
    }
    return manager.IsSceneReady(instanceId);
}

}  // namespace Spark
