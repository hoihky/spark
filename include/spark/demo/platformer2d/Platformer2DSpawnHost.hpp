#pragma once

#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

namespace Spark {

class Platformer2DDemo;
class GameFlowTriggerComponent;
class GameStateComponent;
class GameObject;

/** Dependencies injected into tilemap spawn registry callbacks (demo-local Facade). */
struct Platformer2DSpawnHostContext {
    Platformer2DDemo* demo = nullptr;
    Spark::SharedPtr<Spark::Texture2D> gemTexture{};
    Spark::GameObject* gameFlowObject = nullptr;
    Spark::GameStateComponent* gameState = nullptr;
    float goalHalfW = 1.0F;
    float goalHalfH = 1.0F;
};

class Platformer2DSpawnHost final {
public:
    static void SetContext(const Platformer2DSpawnHostContext& ctx) noexcept;
    static void RegisterHandlers() noexcept;
    static void UnregisterHandlers() noexcept;

private:
    static GameObject* SpawnGem(
            GameWorld& world,
            GameObject& mapOwner,
            const TilemapObjectMarker& marker,
            const TilemapGridFrame& frame);

    static GameObject* SpawnEnemy(
            GameWorld& world,
            GameObject& mapOwner,
            const TilemapObjectMarker& marker,
            const TilemapGridFrame& frame);

    static GameObject* SpawnGoal(
            GameWorld& world,
            GameObject& mapOwner,
            const TilemapObjectMarker& marker,
            const TilemapGridFrame& frame);
};

}  // namespace Spark
