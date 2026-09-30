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
#include "spark/core/Array.hpp"
#include "spark/gameplay/GameplayDataTable.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/IRenderTarget.hpp"
#include "spark/scene/spawn/GameObjectPool.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

class GameObject;
class RenderTexture;
class GameStateComponent;
class IEngineContext;
class TilemapComponent;
class TilemapGameplayGridComponent;
class GridNavAgent2DComponent;
class Scene2DCompositeViewComponent;
class SpriteLighting2DComponent;
class FogOfWar2DComponent;

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

    void OnGemPickedUp(float sceneTimeSeconds) noexcept;
    void BeginGemPickupDissolve(GameObject& gem, float sceneTimeSeconds) noexcept;

private:
    void RebuildAuthoredLevel(GameWorld& world);
    void SetupGameplaySpawnAndMarkers(GameObject& levelRoot, const Vector2& spawnHintWorld) noexcept;
    void ApplyPlayerSpawnCell() noexcept;
    void RefreshHud() noexcept;
    void SyncCameraToPlayer(const TilemapComponent* tilemap) noexcept;
    void TryCollectNearbyGems(GameWorld& world) noexcept;
    void TryReachGoal() noexcept;
    void SaveProgressNow() noexcept;
    void LoadProgressNow() noexcept;
    void CorrectPlayerAgainstBlockedGrid() noexcept;
    void SetupPlayerNavigation() noexcept;
    void RebuildMinimapIfNeeded() noexcept;
    void EnsureMinimapGpuResources(IEngineContext& context) noexcept;
    void SetupMinimapCompositeView(IEngineContext& context) noexcept;
    void SyncMinimapCompositeView() noexcept;
    void SetupProductChaser(GameWorld& world, const GridPathfinder::Cell& spawnCell) noexcept;
    void TickProductChaser(const FrameTiming& timing) noexcept;
    void SetupProductPatrol(GameWorld& world, const GridPathfinder::Cell& spawnCell) noexcept;
    void TickProductPatrolAssignGoals() noexcept;
    void TickProductPatrolMotion(const FrameTiming& timing) noexcept;
    [[nodiscard]] bool IsGemDissolving(const GameObject& gem) const noexcept;
    void TickGemPickupDissolves() noexcept;
    void FinishGemDissolve(GameObject& gem) noexcept;

    DemoRootCollection roots{};
    UniquePtr<SceneManager> sceneManager{};
    UniquePtr<GameObjectPool> gemPool{};
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
    GridNavAgent2DComponent* playerNav = nullptr;
    SpriteLighting2DComponent* playerSpriteFx = nullptr;
    GameObject* chaserObject = nullptr;
    GridNavAgent2DComponent* chaserNav = nullptr;
    Rigidbody2DComponent* chaserRb = nullptr;
    GameObject* patrolObject = nullptr;
    GridNavAgent2DComponent* patrolNav = nullptr;
    Rigidbody2DComponent* patrolRb = nullptr;
    Array<GridPathfinder::Cell> patrolRoute{};
    std::size_t patrolWaypointIndex = 0U;
    bool patrolAwaitingNextGoal = true;
    Scene2DCompositeViewComponent* minimapCompositeView = nullptr;
    FogOfWar2DComponent* fogOfWar = nullptr;
    SharedPtr<Texture2D> minimapTexture{};
    SharedPtr<Texture2D> minimapHudTexture{};
    SharedPtr<RenderTexture> minimapRenderTexture{};
    SharedPtr<IRenderTarget> minimapRenderTargetView{};
    GameplayDataTable gameplayTable{};
    bool useGpuMinimap = true;
    bool minimapDirty = true;
    float moveSpeedScale = 1.0F;
    float chaserSpeedScale = 1.0F;
    bool chaserEnabled = true;
    bool patrolEnabled = true;
    float gemDissolveSeconds = 0.45F;
    float patrolSpeedScale = 0.75F;
    std::uint32_t gemPoolSize = 8U;

    struct GemDissolvePending {
        GameObject* gem = nullptr;
        float startTimeSeconds = 0.0F;
        Vector2 baseScale{1.0F, 1.0F};
    };

    Array<GemDissolvePending> gemDissolvePending{};
    float sceneTimeSeconds = 0.0F;
    bool useKeyboardDrive = true;
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
