#pragma once

#include "spark/core/Utility.hpp"
#include "spark/scene/core/EntityId.hpp"
#include "spark/scene/core/world/WorldQuery.hpp"
#include "spark/scene/core/world/WorldView.hpp"

namespace Spark {

class GameObject;
class GameWorldEntityGraph;
struct GameObjectQueryFilter;
class GameObjectQueryResult;

/** Entity graph: lifecycle, iteration, and query entry (mirrors <c>GameWorldEntityGraph</c>). */
class WorldEntities final : public WorldView {
public:
    using WorldView::WorldView;

    [[nodiscard]] GameObject* Create() { return hostWorld.CreateGameObject(); }

    void Destroy(GameObject* object) { hostWorld.DestroyGameObject(object); }
    void DestroyAtEndOfFrame(GameObject* object) { hostWorld.DestroyGameObjectAtEndOfFrame(object); }
    void DestroyImmediate(GameObject* object) { hostWorld.DestroyGameObjectImmediate(object); }

    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent) {
        return hostWorld.SetParent(child, newParent);
    }

    [[nodiscard]] GameObject* TryGet(EntityId id) const noexcept { return hostWorld.TryGetGameObject(id); }

    [[nodiscard]] WorldQuery Query() const noexcept { return WorldQuery{const_cast<GameWorld&>(hostWorld)}; }

#include "spark/scene/core/world/WorldEntitiesIteration.inl"

    [[nodiscard]] GameObject* FindByTag(const char* tag, const GameObjectQueryFilter& filter = {}) const {
        return hostWorld.FindGameObjectWithTag(tag, filter);
    }

    [[nodiscard]] GameObject* FindByName(const char* name, const GameObjectQueryFilter& filter = {}) const {
        return hostWorld.FindGameObjectByName(name, filter);
    }

    [[nodiscard]] bool IsAlive(const GameObject* object) const noexcept { return hostWorld.IsGameObjectAlive(object); }

    [[nodiscard]] std::size_t Count() const noexcept { return hostWorld.GetGameObjectCount(); }

    void Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter = {}) const {
        hostWorld.CollectGameObjects(out, filter);
    }

    [[nodiscard]] GameObjectQueryResult Select(const GameObjectQueryFilter& filter = {}) const {
        return hostWorld.QueryGameObjects(filter);
    }
};

}  // namespace Spark
