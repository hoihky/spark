#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class TilemapObjectLayerComponent;

/**
 * Instantiates runtime entities for object markers via <c>TilemapObjectSpawnRegistry</c> (F2).
 * Skips <c>TilemapObjectMarkerMode::GizmoOnly</c> markers.
 */
class TilemapObjectSpawnComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapObjectSpawn;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool GetSpawnOnAttach() const noexcept { return spawnOnAttach; }
    void SetSpawnOnAttach(const bool enabled) noexcept { spawnOnAttach = enabled; }

    /** Destroys prior spawns and runs spawn pass again. */
    void RespawnAll(GameObject& owner, GameWorld& world) noexcept;

    void ClearSpawned(GameWorld& world) noexcept;

    [[nodiscard]] const Array<GameObject*>& GetSpawnedObjects() const noexcept { return spawned_; }

    void OnAttach(GameObject& owner) override;
    void OnDetach(GameObject& owner) override;

private:
    void SpawnFromLayers(GameObject& owner, GameWorld& world) noexcept;

    bool spawnOnAttach = true;
    Array<GameObject*> spawned_{};
};

}  // namespace Spark
