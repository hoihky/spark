#pragma once

#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/gameflow/GameFlowCoordinator.hpp"
#include "spark/gameflow/GameFlowPersistentData.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/camera/Camera2D.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/core/SceneSpawn.hpp"
#include "spark/ai/path/GridPathfinder.hpp"

namespace Spark {

class GameObject;
class GameStateComponent;
class IEngineContext;
class TilemapComponent;
class TilemapGameplayGridComponent;

/**
 * Teaching demo for P0 2D product path: authored <c>.sparkscene</c> + TMX, object spawn registry,
 * <c>GameFlowCoordinator</c>, and <c>GameSave</c>.
 */
class GameFlow2DProductPathDemo final {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

    void OnGemPickedUp() noexcept;

private:
    void RebuildAuthoredLevel(GameWorld& world);
    void SetupGameplaySpawnAndMarkers(GameObject& levelRoot, const Vector2& spawnHintWorld) noexcept;
    void ApplyPlayerSpawnCell() noexcept;
    void RefreshHud() noexcept;
    void SyncCameraToPlayer(const TilemapComponent* tilemap) noexcept;
    void ConfigureLevelLayers(GameObject& levelRoot) noexcept;
    void TryCollectNearbyGems(GameWorld& world) noexcept;
    void TryReachGoal() noexcept;
    void SaveProgressNow() noexcept;
    void LoadProgressNow() noexcept;
    void CorrectPlayerAgainstBlockedGrid() noexcept;

    DemoRootCollection roots{};
    UniquePtr<SceneManager> sceneManager{};
    UniquePtr<SceneLoadSession> loadSession{};
    SceneInstanceId levelInstanceId = kInvalidSceneInstanceId;
    GameObject* levelRoot = nullptr;

    GameObject* flowObject = nullptr;
    GameStateComponent* gameState = nullptr;
    GameFlowCoordinator flow{};
    GameFlowPersistentData persistent{};

    GameObject* playerObject = nullptr;
    TransformComponent* playerTr = nullptr;
    Rigidbody2DComponent* playerRb = nullptr;
    BoxCollider2DComponent* playerCollider = nullptr;
    TilemapGameplayGridComponent* walkGrid = nullptr;
    Utf8String saveStatus{};

    Camera2D camera{};
    /** Ortho half-height in world units; scroll wheel adjusts. */
    float cameraHalfExtentY = 8.0F;
    PhysicsSubsystem physics{};
    DemoHelpHud helpHud{};

    int gemsCollected = 0;
    int gemsRequired = 3;
    GameObject* goalObject = nullptr;
    Utf8String levelStatus{};
    char hudLine[256]{};
    Vector3 lastValidPlayerPos{0.0F, 0.0F, 0.08F};
    GridPathfinder::Cell playerSpawnCell{};
    bool playerSpawnResolved = false;
};

}  // namespace Spark
