#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObjectQuery.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/world/WorldView.hpp"

namespace Spark {

class GameObject;

/**
 * Batched object queries (tag, name, component kind, full filter).
 * Prefer over repeated <c>FindGameObjectWithTag</c> when gathering many matches.
 */
class WorldQuery final : public WorldView {
public:
    using WorldView::WorldView;

    void Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter = {}) const;

    [[nodiscard]] GameObjectQueryResult Query(const GameObjectQueryFilter& filter = {}) const;

    /** Appends all matches to <c>out</c> (does not clear <c>out</c>). Returns number appended. */
    [[nodiscard]] std::size_t CollectInto(Array<GameObject*>& out, const GameObjectQueryFilter& filter = {}) const;

    /** Clears <c>out</c> then collects. Returns match count. */
    [[nodiscard]] std::size_t CollectIntoFresh(
            GameObjectQueryResult& out,
            const GameObjectQueryFilter& filter = {}) const;

    [[nodiscard]] std::size_t CollectWithTag(
            const char* tag,
            Array<GameObject*>& out,
            bool includeInactive = false) const;

    [[nodiscard]] std::size_t CollectWithTag(
            const char* tag,
            GameObjectQueryResult& out,
            bool includeInactive = false) const;

    [[nodiscard]] GameObject* FindWithTag(const char* tag, bool includeInactive = false) const;

    [[nodiscard]] GameObject* FindByName(const char* name, bool includeInactive = false) const;

    [[nodiscard]] GameObject* FindFirstWithTag(const char* tag, bool includeInactive = false) const {
        return FindWithTag(tag, includeInactive);
    }

    [[nodiscard]] GameObject* FindFirstWithName(const char* name, bool includeInactive = false) const {
        return FindByName(name, includeInactive);
    }

    [[nodiscard]] std::size_t CollectRoots(Array<GameObject*>& out, bool includeInactive = false) const;

    [[nodiscard]] std::size_t CollectScene(
            SceneInstanceId sceneId,
            Array<GameObject*>& out,
            bool includeInactive = false) const;

    [[nodiscard]] std::size_t CollectWithComponent(
            ComponentKind kind,
            Array<GameObject*>& out,
            bool includeInactive = false) const;

    template<typename... ComponentTypes>
    [[nodiscard]] std::size_t CollectWithComponents(
            Array<GameObject*>& out,
            const GameObjectQueryFilter& filter = {}) const;
};

}  // namespace Spark

#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

template<typename... ComponentTypes>
std::size_t WorldQuery::CollectWithComponents(
        Array<GameObject*>& out,
        const GameObjectQueryFilter& filter) const {
    const std::size_t before = out.GetSize();
    world_.GetEntityGraph().ForEachWithComponents<ComponentTypes...>(
            [&](GameObject* object) { out.PushBack(object); }, filter);
    return out.GetSize() - before;
}

}  // namespace Spark
