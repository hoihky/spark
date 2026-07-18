#include "spark/ecs/GameObjectQuery.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

bool GameObjectPassesQueryFilter(const GameObject* object, const GameObjectQueryFilter& filter) noexcept {
    if (object == nullptr) {
        return false;
    }
    if (!filter.includeInactive && !object->IsActiveInHierarchy()) {
        return false;
    }
    if (filter.rootsOnly && object->GetParent() != nullptr) {
        return false;
    }
    if (filter.sceneInstanceId != kInvalidSceneInstanceId && object->GetSceneInstanceId() != filter.sceneInstanceId) {
        return false;
    }
    if (filter.tagEquals != nullptr) {
        const char* objectTag = object->GetTag().CStr();
        const char* wanted = filter.tagEquals;
        if (wanted[0] == '\0') {
            if (objectTag[0] != '\0') {
                return false;
            }
        } else {
            std::size_t i = 0;
            for (;; ++i) {
                if (wanted[i] != objectTag[i]) {
                    return false;
                }
                if (wanted[i] == '\0') {
                    break;
                }
            }
        }
    }
    if (filter.requiredComponent != ComponentKind::Unknown
            && object->TryGetComponentByKind(filter.requiredComponent) == nullptr) {
        return false;
    }
    return true;
}

}  // namespace Spark
