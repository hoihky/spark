#include "spark/scene/core/SceneInstanceTracker.hpp"

#include "spark/scene/core/SceneManager.hpp"

namespace Spark {

void SceneInstanceTracker::Register(const SceneInstanceId instanceId) noexcept {
    if (instanceId == kInvalidSceneInstanceId) {
        return;
    }
    for (std::size_t i = 0; i < instanceIds.GetSize(); ++i) {
        if (instanceIds[i] == instanceId) {
            return;
        }
    }
    instanceIds.PushBack(instanceId);
}

void SceneInstanceTracker::UnloadAll(SceneManager& manager) noexcept {
    for (std::size_t i = 0; i < instanceIds.GetSize(); ++i) {
        manager.UnloadScene(instanceIds[i]);
    }
    instanceIds.Clear();
}

void SceneInstanceTracker::Clear() noexcept {
    instanceIds.Clear();
}

bool SceneInstanceTracker::Contains(const SceneInstanceId instanceId) const noexcept {
    for (std::size_t i = 0; i < instanceIds.GetSize(); ++i) {
        if (instanceIds[i] == instanceId) {
            return true;
        }
    }
    return false;
}

}  // namespace Spark
