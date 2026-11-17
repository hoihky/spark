#pragma once

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObjectQuery.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/EntityId.hpp"
#include "spark/scene/core/GameWorldAssetStore.hpp"
#include "spark/scene/core/GameWorldEntityGraph.hpp"
#include "spark/scene/core/GameWorldPresentation.hpp"

namespace Spark {

class Font;
class GameObject;
class IEngineContext;
class PhysicsSubsystem;
class WorldAssets;
class GameWorldAssetStore;
class WorldEntities;
class WorldPhysics;
class WorldRendering;
class WorldSimulation;
class WorldQuery;
class WorldGameplay;

/**
 * Facade over entity graph, assets, and presentation.
 * Prefer <c>Entities()</c>, <c>Assets()</c>, <c>QueryObjects()</c>.
 * Layout: <c>include/spark/scene/core/README.md</c> and <c>docs/SCENE_CORE.md</c>.
 */
class GameWorld {
public:
    GameWorld();
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;
    GameWorld(GameWorld&&) = delete;
    GameWorld& operator=(GameWorld&&) = delete;
    ~GameWorld();

    [[nodiscard]] GameObject* CreateGameObject() { return entityGraph.Create(); }
    void DestroyGameObject(GameObject* object) { entityGraph.Destroy(object); }
    void DestroyGameObjectAtEndOfFrame(GameObject* object) { entityGraph.DestroyAtEndOfFrame(object); }
    void DestroyGameObjectImmediate(GameObject* object) { entityGraph.DestroyImmediate(object); }
    void FlushPendingDestroyGameObjects() { entityGraph.FlushPendingDestroys(); }
    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent) {
        return entityGraph.SetParent(child, newParent);
    }

    [[nodiscard]] std::size_t GetGameObjectCount() const noexcept { return entityGraph.GetCount(); }

    [[nodiscard]] GameObject* FindGameObjectById(std::uint64_t id) const noexcept { return entityGraph.FindById(id); }
    [[nodiscard]] GameObject* TryGetGameObject(EntityId id) const noexcept { return entityGraph.TryGet(id); }
    [[nodiscard]] bool IsGameObjectAlive(const GameObject* object) const noexcept { return entityGraph.IsAlive(object); }
    [[nodiscard]] GameObject* FindGameObjectByName(const char* name, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph.FindByName(name, filter);
    }
    [[nodiscard]] GameObject* FindGameObjectWithTag(const char* tag, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph.FindWithTag(tag, filter);
    }
    void CollectGameObjects(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
        entityGraph.Collect(out, filter);
    }
    [[nodiscard]] GameObjectQueryResult QueryGameObjects(const GameObjectQueryFilter& filter = {}) const {
        return entityGraph.Query(filter);
    }

    void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

    [[nodiscard]] VfxSubsystem& GetVfxSubsystem() noexcept { return presentation.Vfx(); }
    [[nodiscard]] const VfxSubsystem& GetVfxSubsystem() const noexcept { return presentation.Vfx(); }
    [[nodiscard]] WindSubsystem& GetWindSubsystem() noexcept { return presentation.Wind(); }
    [[nodiscard]] const WindSubsystem& GetWindSubsystem() const noexcept { return presentation.Wind(); }
    [[nodiscard]] SkinnedAnimationService& GetSkinnedAnimationService() noexcept {
        return presentation.SkinnedAnimation();
    }
    [[nodiscard]] const SkinnedAnimationService& GetSkinnedAnimationService() const noexcept {
        return presentation.SkinnedAnimation();
    }
    [[nodiscard]] SkinnedIkService& GetSkinnedIkService() noexcept { return presentation.SkinnedIk(); }
    [[nodiscard]] const SkinnedIkService& GetSkinnedIkService() const noexcept { return presentation.SkinnedIk(); }
    void SetUiFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return presentation.GetUiFont(); }
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return presentation.GetUiBoldFont(); }

    [[nodiscard]] WorldEntities Entities() noexcept;
    [[nodiscard]] WorldEntities Entities() const noexcept;
    [[nodiscard]] WorldAssets Assets() noexcept;
    [[nodiscard]] WorldAssets Assets() const noexcept;
    [[nodiscard]] WorldSimulation Simulation() noexcept;
    [[nodiscard]] WorldSimulation Simulation() const noexcept;
    [[nodiscard]] WorldRendering Rendering() noexcept;
    [[nodiscard]] WorldRendering Rendering() const noexcept;
    [[nodiscard]] WorldPhysics Physics(PhysicsSubsystem& physics) noexcept;
    [[nodiscard]] WorldPhysics Physics(PhysicsSubsystem& physics) const noexcept;

    [[nodiscard]] WorldQuery QueryObjects() noexcept;
    [[nodiscard]] WorldQuery QueryObjects() const noexcept;
    [[nodiscard]] WorldGameplay Gameplay() noexcept;
    [[nodiscard]] WorldGameplay Gameplay() const noexcept;

    [[nodiscard]] GameWorldEntityGraph& GetEntityGraph() noexcept { return entityGraph; }
    [[nodiscard]] const GameWorldEntityGraph& GetEntityGraph() const noexcept { return entityGraph; }

private:
    friend class GameObject;
    friend class WorldAssets;

    [[nodiscard]] GameWorldAssetStore& AssetStore() noexcept { return assetStore; }
    [[nodiscard]] const GameWorldAssetStore& AssetStore() const noexcept { return assetStore; }

    GameWorldEntityGraph entityGraph;
    GameWorldAssetStore assetStore;
    GameWorldPresentation presentation;
};

}  // namespace Spark

#include "spark/scene/core/GameWorldFacets.hpp"
