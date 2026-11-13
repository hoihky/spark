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
 */
class GameWorld {
public:
    GameWorld();
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;
    GameWorld(GameWorld&&) = delete;
    GameWorld& operator=(GameWorld&&) = delete;
    ~GameWorld();

    [[nodiscard]] GameObject* CreateGameObject() { return entityGraph_.Create(); }
    void DestroyGameObject(GameObject* object) { entityGraph_.Destroy(object); }
    void DestroyGameObjectAtEndOfFrame(GameObject* object) { entityGraph_.DestroyAtEndOfFrame(object); }
    void DestroyGameObjectImmediate(GameObject* object) { entityGraph_.DestroyImmediate(object); }
    void FlushPendingDestroyGameObjects() { entityGraph_.FlushPendingDestroys(); }
    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent) {
        return entityGraph_.SetParent(child, newParent);
    }

    [[nodiscard]] std::size_t GetGameObjectCount() const noexcept { return entityGraph_.GetCount(); }

#include "spark/scene/core/compat/GameWorldEntityApi.inl"

    [[nodiscard]] GameObject* FindGameObjectById(std::uint64_t id) const noexcept { return entityGraph_.FindById(id); }
    [[nodiscard]] GameObject* TryGetGameObject(EntityId id) const noexcept { return entityGraph_.TryGet(id); }
    [[nodiscard]] bool IsGameObjectAlive(const GameObject* object) const noexcept { return entityGraph_.IsAlive(object); }
    [[nodiscard]] GameObject* FindGameObjectByName(const char* name, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindByName(name, filter);
    }
    [[nodiscard]] GameObject* FindGameObjectWithTag(const char* tag, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindWithTag(tag, filter);
    }
    void CollectGameObjects(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
        entityGraph_.Collect(out, filter);
    }
    [[nodiscard]] GameObjectQueryResult QueryGameObjects(const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.Query(filter);
    }

    void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

    [[nodiscard]] VfxSubsystem& GetVfxSubsystem() noexcept { return presentation_.Vfx(); }
    [[nodiscard]] const VfxSubsystem& GetVfxSubsystem() const noexcept { return presentation_.Vfx(); }
    [[nodiscard]] WindSubsystem& GetWindSubsystem() noexcept { return presentation_.Wind(); }
    [[nodiscard]] const WindSubsystem& GetWindSubsystem() const noexcept { return presentation_.Wind(); }
    [[nodiscard]] SkinnedAnimationService& GetSkinnedAnimationService() noexcept {
        return presentation_.SkinnedAnimation();
    }
    [[nodiscard]] const SkinnedAnimationService& GetSkinnedAnimationService() const noexcept {
        return presentation_.SkinnedAnimation();
    }
    [[nodiscard]] SkinnedIkService& GetSkinnedIkService() noexcept { return presentation_.SkinnedIk(); }
    [[nodiscard]] const SkinnedIkService& GetSkinnedIkService() const noexcept { return presentation_.SkinnedIk(); }
    void SetUiFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return presentation_.GetUiFont(); }
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return presentation_.GetUiBoldFont(); }

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

    [[nodiscard]] GameWorldEntityGraph& GetEntityGraph() noexcept { return entityGraph_; }
    [[nodiscard]] const GameWorldEntityGraph& GetEntityGraph() const noexcept { return entityGraph_; }

private:
    friend class GameObject;
    friend class WorldAssets;

    [[nodiscard]] GameWorldAssetStore& AssetStore() noexcept { return assetStore_; }
    [[nodiscard]] const GameWorldAssetStore& AssetStore() const noexcept { return assetStore_; }

    GameWorldEntityGraph entityGraph_;
    GameWorldAssetStore assetStore_;
    GameWorldPresentation presentation_;
};

}  // namespace Spark

#include "spark/scene/core/GameWorldFacets.hpp"
