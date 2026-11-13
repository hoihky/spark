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

    [[nodiscard]] GameObject* Create() { return world_.CreateGameObject(); }

    void Destroy(GameObject* object) { world_.DestroyGameObject(object); }
    void DestroyAtEndOfFrame(GameObject* object) { world_.DestroyGameObjectAtEndOfFrame(object); }
    void DestroyImmediate(GameObject* object) { world_.DestroyGameObjectImmediate(object); }

    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent) {
        return world_.SetParent(child, newParent);
    }

    [[nodiscard]] GameObject* TryGet(EntityId id) const noexcept { return world_.TryGetGameObject(id); }

    [[nodiscard]] WorldQuery Query() const noexcept { return WorldQuery{const_cast<GameWorld&>(world_)}; }

#include "spark/scene/core/world/WorldEntitiesIteration.inl"

    [[nodiscard]] GameObject* FindByTag(const char* tag, const GameObjectQueryFilter& filter = {}) const {
        return world_.FindGameObjectWithTag(tag, filter);
    }

    [[nodiscard]] GameObject* FindByName(const char* name, const GameObjectQueryFilter& filter = {}) const {
        return world_.FindGameObjectByName(name, filter);
    }

    [[nodiscard]] bool IsAlive(const GameObject* object) const noexcept { return world_.IsGameObjectAlive(object); }

    [[nodiscard]] std::size_t Count() const noexcept { return world_.GetGameObjectCount(); }

    void Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter = {}) const {
        world_.CollectGameObjects(out, filter);
    }

    [[nodiscard]] GameObjectQueryResult Select(const GameObjectQueryFilter& filter = {}) const {
        return world_.QueryGameObjects(filter);
    }
};

}  // namespace Spark
