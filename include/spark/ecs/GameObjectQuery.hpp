#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/SceneInstanceId.hpp"

namespace Spark {

class GameObject;

/**
 * Filter for world queries and selection. Default settings preserve legacy behavior for active objects
 * (inactive hierarchy nodes are skipped unless <c>includeInactive</c> is set).
 */
struct GameObjectQueryFilter {
    /** When false (default), objects inactive in the hierarchy are skipped. */
    bool includeInactive = false;
    /** When true, only root objects (no parent) are visited. */
    bool rootsOnly = false;
    /** When not <c>kInvalidSceneInstanceId</c>, only objects with this scene instance id match. */
    SceneInstanceId sceneInstanceId = kInvalidSceneInstanceId;
    /** When non-null, tag must match exactly (UTF-8). Use "" to match only untagged objects. */
    const char* tagEquals = nullptr;
    /** When not <c>Unknown</c>, object must have this component kind. */
    ComponentKind requiredComponent = ComponentKind::Unknown;
};

/** Ordered list of objects produced by <c>GameWorld::QueryGameObjects</c>. */
class GameObjectQueryResult {
public:
    void Clear() noexcept { objects.Clear(); }

    [[nodiscard]] std::size_t GetSize() const noexcept { return objects.GetSize(); }
    [[nodiscard]] bool IsEmpty() const noexcept { return objects.IsEmpty(); }
    [[nodiscard]] GameObject* GetFirst() const noexcept { return objects.IsEmpty() ? nullptr : objects[0]; }
    [[nodiscard]] GameObject* operator[](const std::size_t index) const noexcept { return objects[index]; }
    [[nodiscard]] const Array<GameObject*>& GetObjects() const noexcept { return objects; }

private:
    friend class GameWorld;
    Array<GameObject*> objects;
};

[[nodiscard]] bool GameObjectPassesQueryFilter(const GameObject* object, const GameObjectQueryFilter& filter) noexcept;

namespace GameObjectQueryDetail {

template<typename T>
[[nodiscard]] bool HasComponent(const GameObject* object) noexcept;

template<typename T, typename... Rest>
[[nodiscard]] bool HasAllComponents(const GameObject* object) noexcept {
    return HasComponent<T>(object) && HasAllComponents<Rest...>(object);
}

template<typename T>
[[nodiscard]] bool HasAllComponents(const GameObject* object) noexcept {
    return HasComponent<T>(object);
}

}  // namespace GameObjectQueryDetail

template<typename... ComponentTypes>
[[nodiscard]] bool GameObjectHasAllComponents(const GameObject* object) noexcept {
    if constexpr (sizeof...(ComponentTypes) == 0) {
        return object != nullptr;
    } else {
        return GameObjectQueryDetail::HasAllComponents<ComponentTypes...>(object);
    }
}

}  // namespace Spark

#include "spark/ecs/GameObject.hpp"

namespace Spark::GameObjectQueryDetail {

template<typename T>
bool HasComponent(const GameObject* object) noexcept {
    return object != nullptr && object->GetComponent<T>() != nullptr;
}

}  // namespace Spark::GameObjectQueryDetail
