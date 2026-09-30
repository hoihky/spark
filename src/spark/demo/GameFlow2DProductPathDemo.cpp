#include "spark/demo/GameFlow2DProductPathDemo.hpp"

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/platformer2d/Platformer2DLevelPipeline.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"
#include "spark/ecs/components/gameplay/GameFlowTriggerComponent.hpp"
#include "spark/ecs/components/gameplay/GameStateComponent.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/save/GameSave.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/core/SceneSpawn.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

#include <GLFW/glfw3.h>
#include "spark/scene/tilemap/TilemapGameplayPlacement.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"
#include "spark/scene/tilemap/TilemapObjectLayerCatalog.hpp"
#include "spark/ecs/components/gameplay/FogOfWar2DComponent.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/ai/NavigationSubsystem.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/render/IRenderTargetService.hpp"
#include "spark/render/sprites2d/Scene2DComposite.hpp"
#include "spark/render/sprites2d/Scene2DMinimap.hpp"
#include "spark/render/sprites2d/SpriteFx2D.hpp"
#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/scene/spawn/GameObjectPool.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

struct ProductPathSpawnBindings {
    GameFlow2DProductPathDemo* demo = nullptr;
    GameObjectPool* gemPool = nullptr;
    SharedPtr<Texture2D> gemTexture{};
    GameObject* flowObject = nullptr;
    GameObject** goalObject = nullptr;
    int gemsRequired = 3;
    float sceneTimeSeconds = 0.0F;
};

ProductPathSpawnBindings g_bindings{};

constexpr const char* kP0GemPrefabPath = "prefabs/p0_gem.sparkscene";

void ConfigureSpawnedGem(
        GameObject& gem,
        const Vector2& centerWorld,
        const TilemapGridFrame& frame) {
    gem.GetName() = Utf8String("P0Gem");
    TransformComponent* tr = gem.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        tr = gem.AddComponent<TransformComponent>();
    }
    tr->SetTranslation({centerWorld.x, centerWorld.y, 0.06F});
    const float gemScale = std::max(frame.cellSize * 0.55F, 0.5F);
    tr->SetScale({gemScale, gemScale, 1.0F});
    if (gem.GetComponent<SpriteComponent>() == nullptr && g_bindings.gemTexture) {
        gem.AddComponent<SpriteComponent>(
                g_bindings.gemTexture,
                Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                80);
    }
    if (SpriteLighting2DComponent* lighting = gem.GetComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*lighting, {1.0F, 0.92F, 0.25F, 1.0F}, 2.0F, 1.1F);
    } else if (SpriteLighting2DComponent* added = gem.AddComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*added, {1.0F, 0.92F, 0.25F, 1.0F}, 2.0F, 1.1F);
    }
    if (gem.GetComponent<TriggerVolume2DComponent>() == nullptr) {
        if (TriggerVolume2DComponent* trigger = gem.AddComponent<TriggerVolume2DComponent>(
                    TriggerVolume2DShape::Circle,
                    Vector2{0.5F, 0.5F},
                    Vector2::Zero)) {
            trigger->SetRadius(std::max(frame.cellSize * 0.35F, 0.35F));
        }
    }
    if (gem.GetComponent<Rigidbody2DComponent>() == nullptr) {
        gem.AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
    }
    PickupComponent* pickup = gem.GetComponent<PickupComponent>();
    if (pickup == nullptr) {
        pickup = gem.AddComponent<PickupComponent>();
    }
    pickup->SetItemId("gem");
    pickup->SetDestroyOwnerOnCollect(false);
    pickup->ResetForRespawn();
    pickup->SetOnCollected([&gem](GameObject& /*collector*/, const char*, int) {
        if (g_bindings.demo != nullptr) {
            g_bindings.demo->BeginGemPickupDissolve(gem, g_bindings.sceneTimeSeconds);
        }
    });
}

void BuildPatrolRouteAroundSpawn(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& spawnCell,
        const GridPathfinder::Cell& playerCell,
        Array<GridPathfinder::Cell>& route) noexcept {
    route.Clear();
    Array<GridPathfinder::Cell> reachable{};
    CollectReachableWalkableCells(walk, spawnCell, reachable);
    if (reachable.GetSize() < 4U) {
        return;
    }

    Vector2 centroid{0.0F, 0.0F};
    for (std::size_t i = 0; i < reachable.GetSize(); ++i) {
        const Vector2 w = frame.CellCenterToWorldXY(reachable[i]);
        centroid.x += w.x;
        centroid.y += w.y;
    }
    centroid.x /= static_cast<float>(reachable.GetSize());
    centroid.y /= static_cast<float>(reachable.GetSize());

    struct ScoredCell {
        float angle = 0.0F;
        GridPathfinder::Cell cell{};
    };
    Array<ScoredCell> scored{};
    for (std::size_t i = 0; i < reachable.GetSize(); ++i) {
        const GridPathfinder::Cell& cell = reachable[i];
        if (cell.x == playerCell.x && cell.y == playerCell.y) {
            continue;
        }
        const Vector2 w = frame.CellCenterToWorldXY(cell);
        ScoredCell entry{};
        entry.angle = std::atan2(w.y - centroid.y, w.x - centroid.x);
        entry.cell = cell;
        scored.PushBack(entry);
    }
    if (scored.GetSize() < 4U) {
        return;
    }
    for (std::size_t i = 1; i < scored.GetSize(); ++i) {
        const ScoredCell key = scored[i];
        std::size_t j = i;
        while (j > 0U && scored[j - 1U].angle > key.angle) {
            scored[j] = scored[j - 1U];
            --j;
        }
        scored[j] = key;
    }

    constexpr std::size_t kPatrolStops = 4U;
    for (std::size_t k = 0; k < kPatrolStops; ++k) {
        const std::size_t idx = (k * scored.GetSize()) / kPatrolStops;
        route.PushBack(scored[idx].cell);
    }
}

SharedPtr<Texture2D> MakeGemTexture() {
    Texture2D tex(Utf8String("P0Gem"));
    tex = Texture2D::CreateSolid(12, 12, Vector3{0.95F, 0.82F, 0.18F}, 1.0F);
    return MakeShared<Texture2D>(MoveTemp(tex));
}

SharedPtr<Texture2D> MakePlayerTexture() {
    Texture2D tex(Utf8String("P0Player"));
    tex = Texture2D::CreateSolid(14, 18, Vector3{0.35F, 0.78F, 0.98F}, 1.0F);
    return MakeShared<Texture2D>(MoveTemp(tex));
}

GameObject* SpawnProductGem(
        GameWorld& world,
        GameObject& /*mapOwner*/,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    if (g_bindings.demo == nullptr || !g_bindings.gemTexture) {
        return nullptr;
    }
    const GridPathfinder::Cell cell{marker.cellX, marker.cellY};
    const Vector2 center = frame.CellCenterToWorldXY(cell);
    GameObject* gem = nullptr;
    if (g_bindings.gemPool != nullptr) {
        gem = g_bindings.gemPool->Acquire(world, kP0GemPrefabPath, nullptr);
    }
    if (gem == nullptr) {
        gem = world.CreateGameObject();
    }
    ConfigureSpawnedGem(*gem, center, frame);
    return gem;
}

GameObject* SpawnProductGoal(
        GameWorld& world,
        GameObject& /*mapOwner*/,
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) {
    if (g_bindings.demo == nullptr || g_bindings.flowObject == nullptr) {
        return nullptr;
    }
    const GridPathfinder::Cell goalCell{marker.cellX, marker.cellY};
    const Vector2 center = frame.CellCenterToWorldXY(goalCell);
    GameObject* goal = world.CreateGameObject();
    goal->GetName() = Utf8String("P0Goal");
    TransformComponent* tr = goal->AddComponent<TransformComponent>();
    tr->SetTranslation({center.x, center.y, 0.04F});
    const float cellSize = frame.cellSize;
    goal->AddComponent<TriggerVolume2DComponent>(
            TriggerVolume2DShape::Box,
            Vector2{cellSize * 0.45F, cellSize * 0.45F});
    goal->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
    auto* flow = goal->AddComponent<GameFlowTriggerComponent>();
    flow->SetSource(GameFlowTriggerSource::TriggerEnter);
    flow->SetTargetState(GameFlowState::Victory);
    flow->SetInstigatorNameFilter("P0Player");
    flow->SetStateOwner(g_bindings.flowObject);
    SharedPtr<Texture2D> goalTex = MakeShared<Texture2D>(Utf8String("P0Goal"));
    *goalTex = Texture2D::CreateSolid(16, 16, Vector3{0.92F, 0.22F, 0.35F}, 0.9F);
    goal->AddComponent<SpriteComponent>(
            goalTex,
            Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            70);
    goal->SetActive(false);
    if (g_bindings.goalObject != nullptr) {
        *g_bindings.goalObject = goal;
    }
    return goal;
}

void RegisterSpawnHandlers() {
    TilemapObjectSpawnRegistry::Default().Register("p0_gem", &SpawnProductGem);
    TilemapObjectSpawnRegistry::Default().Register("p0_goal", &SpawnProductGoal);
}

void UnregisterSpawnHandlers() {
    TilemapObjectSpawnRegistry::Default().Unregister("p0_gem");
    TilemapObjectSpawnRegistry::Default().Unregister("p0_goal");
}

/** Kenney <c>sampleMap.tmx</c> town courtyard when <c>PlayerSpawn</c> is unset (cell 15,9). */
constexpr std::int32_t kSampleMapDefaultSpawnCellX = 15;
constexpr std::int32_t kSampleMapDefaultSpawnCellY = 9;

[[nodiscard]] bool TryPlacePlayerOnWalkableCell(
        TransformComponent& playerTr,
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& cell,
        Vector3& lastValidPos) noexcept {
    if (!IsWalkableMapCell(walk, frame, cell.x, cell.y)) {
        return false;
    }
    const Vector2 world = frame.CellCenterToWorldXY(cell);
    playerTr.SetTranslation({world.x, world.y, 0.08F});
    lastValidPos = {world.x, world.y, 0.08F};
    return true;
}

}  // namespace

void GameFlow2DProductPathDemo::ApplyPlayerSpawnCell() noexcept {
    if (playerTr == nullptr || walkGrid == nullptr || levelRoot == nullptr || !playerSpawnResolved) {
        return;
    }
    if (playerRb != nullptr) {
        playerRb->SetVelocity(Vector2::Zero);
    }
    (void)TryPlacePlayerOnWalkableCell(
            *playerTr,
            walkGrid->GetWalkability(),
            walkGrid->GetGridFrame(),
            playerSpawnCell,
            lastValidPlayerPos);
}

void GameFlow2DProductPathDemo::SetupGameplaySpawnAndMarkers(
        GameObject& levelRoot,
        const Vector2& spawnHintWorld) noexcept {
    playerSpawnResolved = false;

    TilemapObjectLayerComponent* objects = levelRoot.GetComponent<TilemapObjectLayerComponent>();
    TilemapGameplayGridComponent* grid = levelRoot.GetComponent<TilemapGameplayGridComponent>();
    if (objects == nullptr || grid == nullptr || walkGrid == nullptr) {
        return;
    }
    const TilemapObjectLayerCatalog authoredMarkers(*objects);
    if (authoredMarkers.CountMarkersByTypeId("p0_gem") >= static_cast<std::size_t>(gemsRequired) &&
        authoredMarkers.ContainsTypeId("p0_goal")) {
        walkGrid->RebakeIfNeeded(levelRoot);
        const TilemapGridFrame& frame = walkGrid->GetGridFrame();
        const IGridWalkability& walk = walkGrid->GetWalkability();
        GridPathfinder::Cell startCell{};
        if (PickSpawnInLargestWalkableRegion(
                    walk, frame, spawnHintWorld, static_cast<std::size_t>(gemsRequired + 2U), startCell)) {
            playerSpawnCell = startCell;
            playerSpawnResolved = true;
        }
        return;
    }
    walkGrid->RebakeIfNeeded(levelRoot);

    const TilemapGridFrame& frame = walkGrid->GetGridFrame();
    const IGridWalkability& walk = walkGrid->GetWalkability();

    const std::size_t minPlayableCells = static_cast<std::size_t>(gemsRequired + 2U);
    GridPathfinder::Cell startCell{};
    if (!PickSpawnInLargestWalkableRegion(
                walk, frame, spawnHintWorld, minPlayableCells, startCell)) {
        return;
    }

    playerSpawnCell = startCell;
    playerSpawnResolved = true;

    Array<GridPathfinder::Cell> reachable{};
    CollectReachableWalkableCells(walk, startCell, reachable);
    if (reachable.GetSize() < minPlayableCells) {
        return;
    }

    if (objects->GetLayerCount() == 0U) {
        objects->AddObjectLayer("P0Objects");
    }
    const std::uint32_t layerIndex = 0U;
    objects->ClearMarkers(layerIndex);

    Array<GridPathfinder::Cell> placements{};
    for (std::size_t i = 0; i < reachable.GetSize(); ++i) {
        const GridPathfinder::Cell& cell = reachable[i];
        if (cell.x == playerSpawnCell.x && cell.y == playerSpawnCell.y) {
            continue;
        }
        placements.PushBack(cell);
    }
    if (placements.GetSize() < static_cast<std::size_t>(gemsRequired + 1U)) {
        return;
    }

    const Vector2 spawnWorld = frame.CellCenterToWorldXY(playerSpawnCell);
    SortCellsByDistanceFromWorld(placements, frame, spawnWorld);

    auto addMarker = [&](const char* typeId, const GridPathfinder::Cell& cell) {
        TilemapObjectMarker marker{};
        marker.typeId = Utf8String(typeId);
        marker.cellX = cell.x;
        marker.cellY = cell.y;
        marker.mode = TilemapObjectMarkerMode::Runtime;
        objects->AddMarker(layerIndex, marker);
    };

    for (int gemIndex = 0; gemIndex < gemsRequired; ++gemIndex) {
        const std::size_t slot = std::min(
                placements.GetSize() - 1U,
                static_cast<std::size_t>(gemIndex + 2U));
        addMarker("p0_gem", placements[slot]);
    }
    const std::size_t goalSlot = std::min(
            placements.GetSize() - 1U,
            static_cast<std::size_t>(gemsRequired + 5U));
    addMarker("p0_goal", placements[goalSlot]);
}

void GameFlow2DProductPathDemo::CorrectPlayerAgainstBlockedGrid() noexcept {
    if (playerTr == nullptr || playerRb == nullptr || playerCollider == nullptr || walkGrid == nullptr) {
        return;
    }
    const TilemapGameplayGrid& grid = walkGrid->GetGrid();
    const TilemapGridFrame& frame = walkGrid->GetGridFrame();

    auto isBlockedAt = [&](const float worldX, const float worldY) noexcept {
        const GridPathfinder::Cell cell = frame.WorldXYToCell({worldX, worldY});
        return !IsWalkableMapCell(grid, frame, cell.x, cell.y);
    };

    CollisionAabb2 playerBox{};
    ComputeBoxCollider2WorldAabb(*playerObject, *playerCollider, playerBox);
    const float cx = (playerBox.minX + playerBox.maxX) * 0.5F;
    const float cy = (playerBox.minY + playerBox.maxY) * 0.5F;
    const float halfW = (playerBox.maxX - playerBox.minX) * 0.5F;
    const float inset = std::min(frame.cellSize * 0.06F, halfW * 0.15F);

    const Vector2 samples[] = {
            {cx, cy},
            {playerBox.minX + inset, playerBox.minY + inset},
            {playerBox.maxX - inset, playerBox.minY + inset},
            {playerBox.minX + inset, playerBox.maxY - inset},
            {playerBox.maxX - inset, playerBox.maxY - inset},
            {cx, playerBox.minY + inset},
            {cx, playerBox.maxY - inset},
            {playerBox.minX + inset, cy},
            {playerBox.maxX - inset, cy},
    };

    bool blocked = false;
    for (const Vector2& sample : samples) {
        if (isBlockedAt(sample.x, sample.y)) {
            blocked = true;
            break;
        }
    }

    Vector3 pos = playerTr->GetLocalTransform().translation;
    if (blocked) {
        playerTr->SetTranslation(lastValidPlayerPos);
        playerRb->SetVelocity(Vector2::Zero);
        return;
    }
    lastValidPlayerPos = pos;
}

void GameFlow2DProductPathDemo::TryCollectNearbyGems(GameWorld& world) noexcept {
    if (playerObject == nullptr || playerTr == nullptr || walkGrid == nullptr) {
        return;
    }
    if (gameState != nullptr && !gameState->IsState(GameFlowState::Playing) &&
            !gameState->IsState(GameFlowState::Intro)) {
        return;
    }
    const Vector3 playerPos = playerTr->GetLocalTransform().translation;
    const float cell = walkGrid->GetGridFrame().cellSize;
    const float collectRadius = cell * 0.58F;
    const float collectRadiusSq = collectRadius * collectRadius;

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr || object == playerObject) {
            return;
        }
        if (IsGemDissolving(*object)) {
            return;
        }
        PickupComponent* pickup = object->GetComponent<PickupComponent>();
        if (pickup == nullptr || pickup->IsCollected()) {
            return;
        }
        const TransformComponent* gemTr = object->GetComponent<TransformComponent>();
        if (gemTr == nullptr) {
            return;
        }
        const Vector3 gemPos = gemTr->GetLocalTransform().translation;
        const float dx = gemPos.x - playerPos.x;
        const float dy = gemPos.y - playerPos.y;
        if (dx * dx + dy * dy > collectRadiusSq) {
            return;
        }
        if (pickup->TryCollect(*playerObject) && pickup->GetDestroyOwnerOnCollect()) {
            object->SetActive(false);
        }
    });
}

void GameFlow2DProductPathDemo::TryReachGoal() noexcept {
    if (goalObject == nullptr || !goalObject->IsActiveInHierarchy() || playerTr == nullptr || walkGrid == nullptr) {
        return;
    }
    if (gemsCollected < gemsRequired) {
        return;
    }
    if (gameState == nullptr || !gameState->IsState(GameFlowState::Playing)) {
        return;
    }
    const TransformComponent* goalTr = goalObject->GetComponent<TransformComponent>();
    if (goalTr == nullptr) {
        return;
    }
    const Vector3 playerPos = playerTr->GetLocalTransform().translation;
    const Vector3 goalPos = goalTr->GetLocalTransform().translation;
    const float cell = walkGrid->GetGridFrame().cellSize;
    const float reachRadius = cell * 0.62F;
    const float reachRadiusSq = reachRadius * reachRadius;
    const float dx = goalPos.x - playerPos.x;
    const float dy = goalPos.y - playerPos.y;
    if (dx * dx + dy * dy <= reachRadiusSq) {
        flow.RequestVictory();
    }
}

void GameFlow2DProductPathDemo::SaveProgressNow() noexcept {
    persistent.progress.activeLevelId = Utf8String("platformer_level");
    flow.SyncProgressFromGameplay(
            gemsCollected,
            gemsRequired,
            0,
            gameState != nullptr && gameState->IsState(GameFlowState::Victory));
    GameSaveSlot slot{};
    persistent.WriteToSlot(slot);
    const bool ok = GameSave::TrySave(GameSave::DefaultSlotPath("p0_demo").CStr(), slot);
    saveStatus = ok ? Utf8String("Saved (F5)") : Utf8String("Save failed");
    RefreshHud();
}

void GameFlow2DProductPathDemo::LoadProgressNow() noexcept {
    flow.LoadProgressFromDisk(GameSave::DefaultSlotPath("p0_demo").CStr());
    gemsCollected = persistent.progress.gemsCollected;
    if (gemsCollected >= gemsRequired && goalObject != nullptr) {
        goalObject->SetActive(true);
    }
    useKeyboardDrive = true;
    if (playerNav != nullptr) {
        playerNav->ClearPath();
    }
    ApplyPlayerSpawnCell();
    if (playerRb != nullptr) {
        playerRb->SetVelocity(Vector2::Zero);
    }
    saveStatus = Utf8String("Loaded (F7)");
    RefreshHud();
}

void GameFlow2DProductPathDemo::SetupPlayerNavigation() noexcept {
    if (playerObject == nullptr || levelRoot == nullptr) {
        playerNav = nullptr;
        return;
    }
    playerNav = playerObject->GetComponent<GridNavAgent2DComponent>();
    if (playerNav == nullptr) {
        playerNav = playerObject->AddComponent<GridNavAgent2DComponent>();
    }
    playerNav->SetGridSourceObject(levelRoot);
    playerNav->SetGoalMode(GridNavGoalMode2D::GridCell);
    playerNav->SetRepathEveryFrame(false);
    playerNav->SetRepathIntervalSeconds(0.2F);
    playerNav->SetSyncToAiAgent(false);
    playerNav->ClearPath();
    useKeyboardDrive = true;
}

void GameFlow2DProductPathDemo::RebuildMinimapIfNeeded() noexcept {
    if (useGpuMinimap || !minimapDirty || walkGrid == nullptr) {
        return;
    }
    if (!minimapTexture) {
        minimapTexture = MakeShared<Texture2D>(Utf8String("P0Minimap"));
    }
    RebuildMinimapTextureFromGameplayGrid(walkGrid->GetGrid(), *minimapTexture);
    minimapDirty = false;
}

void GameFlow2DProductPathDemo::SetupMinimapCompositeView(IEngineContext& context) noexcept {
    if (flowObject == nullptr) {
        return;
    }
    EnsureMinimapGpuResources(context);
    minimapCompositeView = flowObject->GetComponent<Scene2DCompositeViewComponent>();
    if (minimapCompositeView == nullptr) {
        minimapCompositeView = flowObject->AddComponent<Scene2DCompositeViewComponent>();
    }
    minimapCompositeView->SetFeature(Scene2DCompositeFeature::Minimap);
    minimapCompositeView->SetTarget(minimapRenderTexture);
    minimapCompositeView->SetHudTexture(minimapHudTexture);
    minimapCompositeView->SetEnabled(useGpuMinimap);
    SyncMinimapCompositeView();
}

void GameFlow2DProductPathDemo::SyncMinimapCompositeView() noexcept {
    if (minimapCompositeView == nullptr || walkGrid == nullptr) {
        return;
    }
    const Scene2DMinimapOrthoBounds ortho = ComputeMinimapOrthoBoundsSquare(walkGrid->GetGridFrame());
    minimapCompositeView->SetWorldCapture(ortho.worldCenter, ortho.worldHalfHeight);
    minimapCompositeView->SetTarget(minimapRenderTexture);
    minimapCompositeView->SetHudTexture(minimapHudTexture);
    minimapCompositeView->SetEnabled(useGpuMinimap && static_cast<bool>(minimapRenderTexture));
}

void GameFlow2DProductPathDemo::SetupProductChaser(
        GameWorld& world,
        const GridPathfinder::Cell& spawnCell) noexcept {
    if (!chaserEnabled || levelRoot == nullptr || walkGrid == nullptr || !playerSpawnResolved) {
        chaserObject = nullptr;
        chaserNav = nullptr;
        chaserRb = nullptr;
        return;
    }

    const TilemapGridFrame& frame = walkGrid->GetGridFrame();
    Array<GridPathfinder::Cell> reachable{};
    CollectReachableWalkableCells(walkGrid->GetWalkability(), spawnCell, reachable);
    GridPathfinder::Cell chaserCell = spawnCell;
    float bestDist = -1.0F;
    const Vector2 spawnWorld = frame.CellCenterToWorldXY(spawnCell);
    for (std::size_t i = 0; i < reachable.GetSize(); ++i) {
        const GridPathfinder::Cell& cell = reachable[i];
        if (cell.x == playerSpawnCell.x && cell.y == playerSpawnCell.y) {
            continue;
        }
        const Vector2 w = frame.CellCenterToWorldXY(cell);
        const float dx = w.x - spawnWorld.x;
        const float dy = w.y - spawnWorld.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > bestDist) {
            bestDist = d2;
            chaserCell = cell;
        }
    }

    if (chaserObject == nullptr) {
        chaserObject = world.CreateGameObject();
        chaserObject->GetName() = Utf8String("P0Chaser");
        roots.Track(chaserObject);
    }

    const Vector2 center = frame.CellCenterToWorldXY(chaserCell);
    TransformComponent* tr = chaserObject->GetComponent<TransformComponent>();
    if (tr == nullptr) {
        tr = chaserObject->AddComponent<TransformComponent>();
    }
    const float scale = std::max(frame.cellSize * 0.65F, 0.55F);
    tr->SetTranslation({center.x, center.y, 0.07F});
    tr->SetScale({scale, scale, 1.0F});

    SharedPtr<Texture2D> chaserTex = MakeShared<Texture2D>(Utf8String("P0Chaser"));
    *chaserTex = Texture2D::CreateSolid(14, 14, Vector3{0.92F, 0.28F, 0.32F}, 1.0F);
    if (chaserObject->GetComponent<SpriteComponent>() == nullptr) {
        chaserObject->AddComponent<SpriteComponent>(
                chaserTex,
                Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                90);
    }
    if (SpriteLighting2DComponent* fx = chaserObject->GetComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*fx, {0.15F, 0.05F, 0.08F, 1.0F}, 2.5F, 1.0F);
    } else if (SpriteLighting2DComponent* added = chaserObject->AddComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*added, {0.15F, 0.05F, 0.08F, 1.0F}, 2.5F, 1.0F);
    }

    if (chaserObject->GetComponent<BoxCollider2DComponent>() == nullptr) {
        auto* col = chaserObject->AddComponent<BoxCollider2DComponent>();
        col->SetHalfExtents({scale * 0.45F, scale * 0.45F});
    }
    chaserRb = chaserObject->GetComponent<Rigidbody2DComponent>();
    if (chaserRb == nullptr) {
        chaserRb = chaserObject->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 0.0F);
        chaserRb->SetGravityScale(0.0F);
    }

    chaserNav = chaserObject->GetComponent<GridNavAgent2DComponent>();
    if (chaserNav == nullptr) {
        chaserNav = chaserObject->AddComponent<GridNavAgent2DComponent>();
    }
    chaserNav->SetGridSourceObject(levelRoot);
    chaserNav->SetGoalMode(GridNavGoalMode2D::TargetObject);
    chaserNav->SetGoalTarget(playerObject);
    chaserNav->SetRepathEveryFrame(false);
    chaserNav->SetRepathIntervalSeconds(0.35F);
    chaserNav->SetSyncToAiAgent(false);
    chaserNav->RequestRepath();
}

bool GameFlow2DProductPathDemo::IsGemDissolving(const GameObject& gem) const noexcept {
    for (std::size_t i = 0; i < gemDissolvePending.GetSize(); ++i) {
        if (gemDissolvePending[i].gem == &gem) {
            return true;
        }
    }
    return false;
}

void GameFlow2DProductPathDemo::BeginGemPickupDissolve(GameObject& gem, const float sceneTimeSecondsIn) noexcept {
    if (IsGemDissolving(gem)) {
        return;
    }
    OnGemPickedUp(sceneTimeSecondsIn);

    GemDissolvePending pending{};
    pending.gem = &gem;
    pending.startTimeSeconds = sceneTimeSecondsIn;
    if (const TransformComponent* tr = gem.GetComponent<TransformComponent>()) {
        const Vector3 scale = tr->GetLocalTransform().scale;
        pending.baseScale = {scale.x, scale.y};
    }
    gemDissolvePending.PushBack(pending);

    if (PickupComponent* pickup = gem.GetComponent<PickupComponent>()) {
        pickup->SetAutoCollectOnTriggerEnter(false);
    }

    if (SpriteLighting2DComponent* lighting = gem.GetComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyDissolveProgress(*lighting, 0.0F);
    } else if (SpriteLighting2DComponent* added = gem.AddComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyDissolveProgress(*added, 0.0F);
    }
}

void GameFlow2DProductPathDemo::FinishGemDissolve(GameObject& gem) noexcept {
    if (g_bindings.gemPool != nullptr) {
        if (PickupComponent* pickup = gem.GetComponent<PickupComponent>()) {
            pickup->ResetForRespawn();
            pickup->SetAutoCollectOnTriggerEnter(true);
        }
        if (TransformComponent* tr = gem.GetComponent<TransformComponent>()) {
            tr->SetScale({1.0F, 1.0F, 1.0F});
        }
        if (SpriteLighting2DComponent* lighting = gem.GetComponent<SpriteLighting2DComponent>()) {
            SpriteFx2D::ApplyOutline(*lighting, {1.0F, 0.92F, 0.25F, 1.0F}, 2.0F, 1.1F);
        }
        g_bindings.gemPool->Release(kP0GemPrefabPath, &gem);
    } else {
        gem.SetActive(false);
    }
}

void GameFlow2DProductPathDemo::TickGemPickupDissolves() noexcept {
    if (gemDissolvePending.IsEmpty()) {
        return;
    }
    const float duration = std::max(gemDissolveSeconds, 0.05F);
    std::size_t write = 0U;
    for (std::size_t i = 0; i < gemDissolvePending.GetSize(); ++i) {
        GemDissolvePending& pending = gemDissolvePending[i];
        GameObject* gem = pending.gem;
        if (gem == nullptr || !gem->IsActiveInHierarchy()) {
            continue;
        }
        const float t = (sceneTimeSeconds - pending.startTimeSeconds) / duration;
        const float progress = std::clamp(t, 0.0F, 1.0F);
        if (SpriteLighting2DComponent* lighting = gem->GetComponent<SpriteLighting2DComponent>()) {
            SpriteFx2D::ApplyDissolveProgress(*lighting, progress);
        }
        if (TransformComponent* tr = gem->GetComponent<TransformComponent>()) {
            const float shrink = 1.0F - progress * 0.35F;
            tr->SetScale({pending.baseScale.x * shrink, pending.baseScale.y * shrink, 1.0F});
        }
        if (t >= 1.0F) {
            FinishGemDissolve(*gem);
            continue;
        }
        if (write != i) {
            gemDissolvePending[write] = pending;
        }
        ++write;
    }
    gemDissolvePending.Resize(write);
}

void GameFlow2DProductPathDemo::SetupProductPatrol(
        GameWorld& world,
        const GridPathfinder::Cell& spawnCell) noexcept {
    patrolRoute.Clear();
    patrolWaypointIndex = 0U;
    patrolAwaitingNextGoal = true;

    if (!patrolEnabled || levelRoot == nullptr || walkGrid == nullptr || !playerSpawnResolved) {
        if (patrolObject != nullptr) {
            patrolObject->SetActive(false);
        }
        return;
    }

    const TilemapGridFrame& frame = walkGrid->GetGridFrame();
    BuildPatrolRouteAroundSpawn(
            walkGrid->GetWalkability(), frame, spawnCell, playerSpawnCell, patrolRoute);
    if (patrolRoute.IsEmpty()) {
        return;
    }

    if (patrolObject == nullptr) {
        patrolObject = world.CreateGameObject();
        patrolObject->GetName() = Utf8String("P0Patrol");
        roots.Track(patrolObject);
    }

    const Vector2 start = frame.CellCenterToWorldXY(patrolRoute[0U]);
    TransformComponent* tr = patrolObject->GetComponent<TransformComponent>();
    if (tr == nullptr) {
        tr = patrolObject->AddComponent<TransformComponent>();
    }
    const float scale = std::max(frame.cellSize * 0.62F, 0.52F);
    tr->SetTranslation({start.x, start.y, 0.065F});
    tr->SetScale({scale, scale, 1.0F});

    SharedPtr<Texture2D> patrolTex = MakeShared<Texture2D>(Utf8String("P0Patrol"));
    *patrolTex = Texture2D::CreateSolid(14, 14, Vector3{0.32F, 0.78F, 0.55F}, 1.0F);
    if (patrolObject->GetComponent<SpriteComponent>() == nullptr) {
        patrolObject->AddComponent<SpriteComponent>(
                patrolTex,
                Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                85);
    }
    if (SpriteLighting2DComponent* fx = patrolObject->GetComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*fx, {0.05F, 0.12F, 0.10F, 1.0F}, 2.0F, 1.0F);
    } else if (SpriteLighting2DComponent* added = patrolObject->AddComponent<SpriteLighting2DComponent>()) {
        SpriteFx2D::ApplyOutline(*added, {0.05F, 0.12F, 0.10F, 1.0F}, 2.0F, 1.0F);
    }

    if (patrolObject->GetComponent<BoxCollider2DComponent>() == nullptr) {
        auto* col = patrolObject->AddComponent<BoxCollider2DComponent>();
        col->SetHalfExtents({scale * 0.42F, scale * 0.42F});
    }
    patrolRb = patrolObject->GetComponent<Rigidbody2DComponent>();
    if (patrolRb == nullptr) {
        patrolRb = patrolObject->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 0.0F);
        patrolRb->SetGravityScale(0.0F);
    }

    patrolNav = patrolObject->GetComponent<GridNavAgent2DComponent>();
    if (patrolNav == nullptr) {
        patrolNav = patrolObject->AddComponent<GridNavAgent2DComponent>();
    }
    patrolNav->SetGridSourceObject(levelRoot);
    patrolNav->SetGoalMode(GridNavGoalMode2D::GridCell);
    patrolNav->SetRepathEveryFrame(false);
    patrolNav->SetRepathIntervalSeconds(0.5F);
    patrolNav->SetSyncToAiAgent(false);
    patrolObject->SetActive(true);
    patrolWaypointIndex = 1U % patrolRoute.GetSize();
    patrolAwaitingNextGoal = true;
    patrolNav->ClearPath();
}

void GameFlow2DProductPathDemo::TickProductPatrolAssignGoals() noexcept {
    if (!patrolEnabled || patrolObject == nullptr || patrolNav == nullptr || patrolRoute.IsEmpty() ||
        !patrolObject->IsActiveInHierarchy()) {
        return;
    }
    if (!patrolAwaitingNextGoal || patrolNav->HasPath()) {
        return;
    }
    patrolNav->SetGoalMode(GridNavGoalMode2D::GridCell);
    patrolNav->SetGoalCell(patrolRoute[patrolWaypointIndex]);
    patrolNav->RequestRepath();
    patrolAwaitingNextGoal = false;
}

void GameFlow2DProductPathDemo::TickProductPatrolMotion(const FrameTiming& timing) noexcept {
    if (!patrolEnabled || patrolObject == nullptr || patrolNav == nullptr || patrolRb == nullptr ||
        patrolRoute.IsEmpty() || !patrolObject->IsActiveInHierarchy()) {
        return;
    }
    if (!patrolNav->HasPath()) {
        if (!patrolAwaitingNextGoal) {
            patrolAwaitingNextGoal = true;
            patrolWaypointIndex = (patrolWaypointIndex + 1U) % patrolRoute.GetSize();
        }
        patrolRb->SetVelocity(Vector2::Zero);
        return;
    }
    TransformComponent* tr = patrolObject->GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    const float cell = walkGrid != nullptr ? walkGrid->GetGridFrame().cellSize : 1.0F;
    const float speed = cell * 1.65F * patrolSpeedScale;
    const float arrive = cell * 0.14F;
    if (!ApplyGridNavAgent2DRigidbodyMotion(*patrolNav, *tr, *patrolRb, speed, arrive, timing.deltaTimeSeconds)) {
        patrolRb->SetVelocity(Vector2::Zero);
    }
}

void GameFlow2DProductPathDemo::TickProductChaser(const FrameTiming& timing) noexcept {
    if (!chaserEnabled || chaserObject == nullptr || chaserNav == nullptr || chaserRb == nullptr ||
        playerObject == nullptr || !chaserObject->IsActiveInHierarchy()) {
        return;
    }
    TransformComponent* tr = chaserObject->GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    const float cell = walkGrid != nullptr ? walkGrid->GetGridFrame().cellSize : 1.0F;
    const float speed = cell * 2.1F * chaserSpeedScale;
    const float arrive = cell * 0.14F;
    if (!ApplyGridNavAgent2DSteeringMotion(*chaserNav, *tr, *chaserRb, speed, arrive, timing.deltaTimeSeconds)) {
        chaserRb->SetVelocity(Vector2::Zero);
    }
}

void GameFlow2DProductPathDemo::EnsureMinimapGpuResources(IEngineContext& context) noexcept {
    if (!useGpuMinimap) {
        return;
    }
    if (!minimapRenderTexture) {
        minimapRenderTexture = CreateMinimapRenderTexture(384U);
    }
    if (!minimapHudTexture) {
        minimapHudTexture = CreateMinimapHudPlaceholderTexture(384U);
        minimapTexture = minimapHudTexture;
    }
    if (!minimapRenderTargetView && minimapRenderTexture) {
        if (IRenderTargetService* targets = context.TryGetRenderTargetService()) {
            minimapRenderTargetView = targets->CreateRenderTarget(minimapRenderTexture);
        }
    }
}

void GameFlow2DProductPathDemo::RebuildAuthoredLevel(GameWorld& world) {
    if (loadSession == nullptr || sceneManager == nullptr) {
        return;
    }
    if (levelInstanceId != kInvalidSceneInstanceId) {
        sceneManager->UnloadScene(levelInstanceId);
        levelInstanceId = kInvalidSceneInstanceId;
        levelRoot = nullptr;
        walkGrid = nullptr;
    }

    Utf8String scenePath = ScenePathResolver::ResolveReadablePath("scenes/platformer_level.sparkscene");
    if (scenePath.IsEmpty()) {
        scenePath = ScenePathResolver::BuildRuntimePath("scenes", "platformer_level.sparkscene");
    }
    const Platformer2DLevelLoadResult loaded =
            Platformer2DLevelPipeline::TryLoad(world, *loadSession, roots, scenePath.CStr());
    if (!loaded.success || loaded.levelRoot == nullptr) {
        levelStatus = loaded.message;
        return;
    }
    levelInstanceId = loaded.instanceId;
    levelRoot = loaded.levelRoot;
    flow.SetActiveLevelScenePath(scenePath.CStr());
    levelStatus = Utf8String("Level: platformer_level.sparkscene + sampleMap.tmx");

    if (TilemapGameplayGridComponent* grid = levelRoot->GetComponent<TilemapGameplayGridComponent>()) {
        walkGrid = grid;
    } else {
        walkGrid = levelRoot->AddComponent<TilemapGameplayGridComponent>();
    }
    if (TilemapMapSourceComponent* mapSource = levelRoot->GetComponent<TilemapMapSourceComponent>()) {
        mapSource->SetImportOnAttach(false);
        (void)mapSource->ImportNow(*levelRoot, world);
    }

    if (TilemapComponent* tilemapLayers = levelRoot->GetComponent<TilemapComponent>(); tilemapLayers != nullptr) {
        ApplyDefaultGameplayLayerFlags(*tilemapLayers);
    }

    walkGrid->SetWalkRule(TilemapGameplayWalkRule::CollisionAligned);
    walkGrid->SetAutoRebake(true);
    walkGrid->RequestRebake();
    walkGrid->RebakeIfNeeded(*levelRoot);

    const SceneSpawnPose levelSpawnPose = FindSpawnPoint(world, "Player");
    const TilemapGridFrame& gridFrame = walkGrid->GetGridFrame();
    Vector2 spawnHint = gridFrame.CellCenterToWorldXY({kSampleMapDefaultSpawnCellX, kSampleMapDefaultSpawnCellY});
    if (levelSpawnPose.found) {
        spawnHint = {levelSpawnPose.position.x, levelSpawnPose.position.y};
    }
    SetupGameplaySpawnAndMarkers(*levelRoot, spawnHint);

    if (playerSpawnResolved && playerTr != nullptr) {
        char spawnDebug[96]{};
        std::snprintf(
                spawnDebug,
                sizeof(spawnDebug),
                "Spawn cell (%d,%d) hint %.1f,%.1f",
                playerSpawnCell.x,
                playerSpawnCell.y,
                spawnHint.x,
                spawnHint.y);
        levelStatus = Utf8String(spawnDebug);
    }

    g_bindings.demo = this;
    g_bindings.gemPool = gemPool.Get();
    g_bindings.gemTexture = MakeGemTexture();
    g_bindings.flowObject = flowObject;
    g_bindings.goalObject = &goalObject;
    g_bindings.gemsRequired = gemsRequired;
    goalObject = nullptr;
    RegisterSpawnHandlers();

    if (gemPool != nullptr) {
        gemPool->Prewarm(world, kP0GemPrefabPath, gemPoolSize, levelRoot);
    }

    if (TilemapObjectSpawnComponent* spawn = levelRoot->GetComponent<TilemapObjectSpawnComponent>()) {
        spawn->SetSpawnOnAttach(false);
        spawn->RespawnAll(*levelRoot, world);
    }

    physics.GetQueries2D().RebuildStatics(world);

    SetupPlayerNavigation();
    SetupProductChaser(world, playerSpawnCell);
    SetupProductPatrol(world, playerSpawnCell);
    fogOfWar = levelRoot->GetComponent<FogOfWar2DComponent>();
    if (fogOfWar == nullptr) {
        fogOfWar = levelRoot->AddComponent<FogOfWar2DComponent>();
    }
    fogOfWar->SetRevealTarget(playerObject);
    fogOfWar->SyncGridFromOwner(*levelRoot);
    minimapDirty = true;
    RebuildMinimapIfNeeded();

    ApplyPlayerSpawnCell();
    if (playerTr != nullptr && walkGrid != nullptr && !playerSpawnResolved) {
        levelStatus = Utf8String("No reachable walkable spawn — check TMX / PlayerSpawn.");
    }

    const TilemapComponent* tilemap = levelRoot->GetComponent<TilemapComponent>();
    if (playerTr != nullptr && tilemap != nullptr) {
        const float cell = tilemap->GetTileWorldSize();
        const float spriteSize = std::max(cell * 0.75F, 0.65F);
        playerTr->SetScale({spriteSize, spriteSize, 1.0F});
        cameraHalfExtentY = cell * 7.0F;
        if (playerCollider != nullptr) {
            const float half = std::max(cell * 0.40F, 0.34F);
            playerCollider->SetHalfExtents({half, half});
        }
        physics.SetBroadPhaseCellSize2D(std::max(cell * 0.5F, 0.25F));
    }
    SyncCameraToPlayer(tilemap);
    SyncMinimapCompositeView();

    gemsCollected = persistent.progress.gemsCollected;
    if (gemsCollected >= gemsRequired && goalObject != nullptr) {
        goalObject->SetActive(true);
    }
}

void GameFlow2DProductPathDemo::SyncCameraToPlayer(const TilemapComponent* tilemap) noexcept {
    if (playerTr == nullptr) {
        return;
    }
    const Vector3 p = playerTr->GetLocalTransform().translation;
    camera.position = {p.x, p.y, 0.0F};
    const float cell = tilemap != nullptr ? tilemap->GetTileWorldSize() : 1.0F;
    if (tilemap != nullptr) {
        const float mapH = static_cast<float>(tilemap->GetMapHeight()) * cell;
        const float maxHalf = std::max(cell * 6.0F, mapH * 0.48F);
        cameraHalfExtentY = std::clamp(cameraHalfExtentY, cell * 2.5F, maxHalf);
    }
    camera.halfExtentY = cameraHalfExtentY;
}

void GameFlow2DProductPathDemo::OnGemPickedUp(const float sceneTimeAtPickup) noexcept {
    if (playerSpriteFx != nullptr) {
        SpriteFx2D::ApplyHitFlashAtSceneTime(
                *playerSpriteFx,
                {1.0F, 0.95F, 0.45F, 1.25F},
                0.35F,
                sceneTimeAtPickup);
    }
    ++gemsCollected;
    persistent.progress.gemsCollected = gemsCollected;
    if (gemsCollected >= gemsRequired && goalObject != nullptr) {
        goalObject->SetActive(true);
    }
    flow.SyncProgressFromGameplay(gemsCollected, gemsRequired, 0, false);
    RefreshHud();
}

void GameFlow2DProductPathDemo::RefreshHud() noexcept {
    const char* stateName = "Unknown";
    if (gameState != nullptr) {
        switch (gameState->GetState()) {
            case GameFlowState::Intro:
                stateName = "Intro";
                break;
            case GameFlowState::Playing:
                stateName = "Playing";
                break;
            case GameFlowState::Paused:
                stateName = "Paused";
                break;
            case GameFlowState::Victory:
                stateName = "Victory";
                break;
            case GameFlowState::Defeat:
                stateName = "Defeat";
                break;
        }
    }
    const char* moveHint = (gameState != nullptr && gameState->IsState(GameFlowState::Intro))
            ? " | WASD or Enter/Space to start"
            : "";
    std::snprintf(
            hudLine,
            sizeof(hudLine),
            "P0Player (cyan) | GameState: %s | Gems %d/%d | %s | %s%s",
            stateName,
            gemsCollected,
            gemsRequired,
            saveStatus.IsEmpty() ? "F5 save / F7 load" : saveStatus.CStr(),
            levelStatus.CStr(),
            moveHint);
    helpHud.SetDetail(hudLine);
}

void GameFlow2DProductPathDemo::Load(GameWorld& world, IEngineContext& context) {
    Unload(world);
    gemsCollected = 0;
    gemsRequired = 3;
    moveSpeedScale = 1.0F;
    chaserSpeedScale = 0.92F;
    chaserEnabled = true;
    patrolEnabled = true;
    patrolSpeedScale = 0.75F;
    gemDissolveSeconds = 0.45F;
    gemPoolSize = 8U;
    persistent = GameFlowPersistentData{};
    flow.LoadProgressFromDisk(GameSave::DefaultSlotPath("p0_demo").CStr());

    Utf8String gameplayPath = ScenePathResolver::ResolveReadablePath("gameplay/p0_demo.sparkgameplay");
    if (gameplayPath.IsEmpty()) {
        gameplayPath = ScenePathResolver::BuildRuntimePath("gameplay", "p0_demo.sparkgameplay");
    }
    if (gameplayTable.TryLoad(gameplayPath.CStr())) {
        gemsRequired = gameplayTable.GetInt("p0.gems_required", gemsRequired);
        gemPoolSize = static_cast<std::uint32_t>(gameplayTable.GetInt("p0.gem_pool_size", static_cast<int>(gemPoolSize)));
        moveSpeedScale = gameplayTable.GetFloat("p0.move_speed_scale", moveSpeedScale);
        chaserEnabled = gameplayTable.GetInt("p0.chaser_enabled", chaserEnabled ? 1 : 0) != 0;
        chaserSpeedScale = gameplayTable.GetFloat("p0.chaser_speed_scale", chaserSpeedScale);
        patrolEnabled = gameplayTable.GetInt("p0.patrol_enabled", patrolEnabled ? 1 : 0) != 0;
        patrolSpeedScale = gameplayTable.GetFloat("p0.patrol_speed_scale", patrolSpeedScale);
        gemDissolveSeconds = gameplayTable.GetFloat("p0.gem_dissolve_seconds", gemDissolveSeconds);
    }
    sceneTimeSeconds = 0.0F;

    sceneManager = MakeUnique<SceneManager>(world);
    loadSession = MakeUnique<SceneLoadSession>(*sceneManager);
    gemPool = MakeUnique<GameObjectPool>(*sceneManager);

    flowObject = world.CreateGameObject();
    flowObject->GetName() = Utf8String("P0GameFlow");
    gameState = flowObject->AddComponent<GameStateComponent>(GameFlowState::Intro);
    flow.Bind(gameState, &persistent);
    gameState->SetOnTransition([this](GameFlowState, GameFlowState next, GameObject&) {
        if (next == GameFlowState::Victory) {
            flow.SyncProgressFromGameplay(gemsCollected, gemsRequired, 0, true);
            flow.SaveProgressToDisk(GameSave::DefaultSlotPath("p0_demo").CStr());
            RefreshHud();
        }
    });
    roots.Track(flowObject);

    SharedPtr<Texture2D> playerTex = MakePlayerTexture();
    world.RegisterTexture(playerTex, "spark/p0_demo/player");

    playerObject = world.CreateGameObject();
    playerObject->GetName() = Utf8String("P0Player");
    playerTr = playerObject->AddComponent<TransformComponent>();
    playerTr->SetUniformScale(0.55F);
    playerObject->AddComponent<SpriteComponent>(
            playerTex,
            Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            100);
    playerCollider = playerObject->AddComponent<BoxCollider2DComponent>();
    playerCollider->SetHalfExtents({0.35F, 0.35F});
    playerRb = playerObject->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 0.0F);
    playerRb->SetGravityScale(0.0F);
    playerSpriteFx = playerObject->AddComponent<SpriteLighting2DComponent>();
    roots.Track(playerObject);

    RebuildAuthoredLevel(world);
    EnsureMinimapGpuResources(context);
    SetupMinimapCompositeView(context);

    helpHud.Mount(world, "2D P0 product path");
    helpHud.SetControlHints(
            "WASD | click path | green patrol loop | red chaser | gem dissolve+flash | P pause | R reload");
    RefreshHud();
    context.GetInput().SetCursorCaptured(false);
}

void GameFlow2DProductPathDemo::Unload(GameWorld& world) {
    helpHud.Unmount(world);
    UnregisterSpawnHandlers();
    g_bindings = ProductPathSpawnBindings{};
    if (sceneManager != nullptr && levelInstanceId != kInvalidSceneInstanceId) {
        sceneManager->UnloadScene(levelInstanceId);
    }
    levelInstanceId = kInvalidSceneInstanceId;
    levelRoot = nullptr;
    walkGrid = nullptr;
    loadSession.Reset();
    gemPool.Reset();
    sceneManager.Reset();
    flowObject = nullptr;
    gameState = nullptr;
    playerObject = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    playerCollider = nullptr;
    playerSpawnResolved = false;
    playerNav = nullptr;
    playerSpriteFx = nullptr;
    chaserObject = nullptr;
    chaserNav = nullptr;
    chaserRb = nullptr;
    patrolObject = nullptr;
    patrolNav = nullptr;
    patrolRb = nullptr;
    patrolRoute.Clear();
    patrolWaypointIndex = 0U;
    patrolAwaitingNextGoal = true;
    gemDissolvePending.Clear();
    minimapCompositeView = nullptr;
    minimapTexture.Reset();
    minimapHudTexture.Reset();
    minimapRenderTexture.Reset();
    minimapRenderTargetView.Reset();
    minimapDirty = true;
    useKeyboardDrive = true;
    saveStatus.Clear();
    roots.DestroyAll(world);
    physics = PhysicsSubsystem{};
    gemsCollected = 0;
    levelStatus.Clear();
}

void GameFlow2DProductPathDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    const float dt = timing.deltaTimeSeconds;
    sceneTimeSeconds += dt;
    g_bindings.sceneTimeSeconds = sceneTimeSeconds;
    IInput& input = context.GetInput();

    const float scroll = input.GetScrollDeltaY();
    if (walkGrid != nullptr) {
        const float cell = walkGrid->GetGridFrame().cellSize;
        if (std::abs(scroll) > 0.001F) {
            const float zoomFactor = 1.0F - scroll * 0.14F;
            cameraHalfExtentY = std::clamp(cameraHalfExtentY * zoomFactor, cell * 2.0F, cell * 22.0F);
        }
        if (input.IsKeyDown(GLFW_KEY_EQUAL) || input.IsKeyDown(GLFW_KEY_KP_ADD)) {
            cameraHalfExtentY = std::max(cell * 2.0F, cameraHalfExtentY - cell * 2.5F * dt);
        }
        if (input.IsKeyDown(GLFW_KEY_MINUS) || input.IsKeyDown(GLFW_KEY_KP_SUBTRACT)) {
            cameraHalfExtentY = std::min(cell * 22.0F, cameraHalfExtentY + cell * 2.5F * dt);
        }
        if (std::abs(scroll) > 0.001F || input.IsKeyDown(GLFW_KEY_EQUAL) || input.IsKeyDown(GLFW_KEY_MINUS) ||
            input.IsKeyDown(GLFW_KEY_KP_ADD) || input.IsKeyDown(GLFW_KEY_KP_SUBTRACT)) {
            SyncCameraToPlayer(levelRoot != nullptr ? levelRoot->GetComponent<TilemapComponent>() : nullptr);
        }
    }

    if (gameState != nullptr && gameState->IsState(GameFlowState::Intro)) {
        if (input.IsKeyPressedThisFrame(GLFW_KEY_ENTER) || input.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
            flow.RequestPlaying();
            RefreshHud();
        }
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_P)) {
        flow.TogglePause();
        RefreshHud();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_F5)) {
        SaveProgressNow();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_F7)) {
        LoadProgressNow();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_R)) {
        gemsCollected = 0;
        persistent.progress.gemsCollected = 0;
        RebuildAuthoredLevel(world);
        if (gameState != nullptr) {
            gameState->RequestState(GameFlowState::Playing);
        }
        RefreshHud();
    }

    const bool paused = gameState != nullptr && gameState->IsState(GameFlowState::Paused);
    const bool canMove = gameState != nullptr &&
            (gameState->IsState(GameFlowState::Playing) || gameState->IsState(GameFlowState::Intro)) && !paused;

    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }

    if (canMove && walkGrid != nullptr) {
        TickProductPatrolAssignGoals();
        ProcessGridNavAgents2D(world, timing.deltaTimeSeconds);
        TickProductPatrolMotion(timing);
        TickProductChaser(timing);
        SyncMinimapCompositeView();
    }

    if (canMove && walkGrid != nullptr && playerNav != nullptr) {
        if (input.IsMouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT)) {
            float mx = 0.0F;
            float my = 0.0F;
            input.GetCursorFramebufferPixels(mx, my, fbW, fbH);
            GridPathfinder::Cell goal{};
            if (TryPickWalkableGridCellFromScreen(
                        camera,
                        walkGrid->GetGridFrame(),
                        walkGrid->GetWalkability(),
                        static_cast<float>(fbW),
                        static_cast<float>(fbH),
                        mx,
                        my,
                        goal)) {
                playerNav->SetGoalMode(GridNavGoalMode2D::GridCell);
                playerNav->SetGoalCell(goal);
                playerNav->RequestRepath();
                useKeyboardDrive = false;
                if (gameState->IsState(GameFlowState::Intro)) {
                    flow.RequestPlaying();
                }
            }
        }
    }

    if (canMove && playerRb != nullptr && walkGrid != nullptr && playerTr != nullptr) {
        float moveX = 0.0F;
        float moveY = 0.0F;
        if (input.IsKeyDown(GLFW_KEY_A) || input.IsKeyDown(GLFW_KEY_LEFT)) {
            moveX -= 1.0F;
        }
        if (input.IsKeyDown(GLFW_KEY_D) || input.IsKeyDown(GLFW_KEY_RIGHT)) {
            moveX += 1.0F;
        }
        if (input.IsKeyDown(GLFW_KEY_W) || input.IsKeyDown(GLFW_KEY_UP)) {
            moveY += 1.0F;
        }
        if (input.IsKeyDown(GLFW_KEY_S) || input.IsKeyDown(GLFW_KEY_DOWN)) {
            moveY -= 1.0F;
        }
        if (input.IsGamepadPresent()) {
            moveX += input.GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X);
            moveY -= input.GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y);
        }
        const bool keyboardIntent = std::abs(moveX) > 0.01F || std::abs(moveY) > 0.01F;
        if (keyboardIntent) {
            useKeyboardDrive = true;
            if (playerNav != nullptr) {
                playerNav->ClearPath();
            }
        }
        if (gameState->IsState(GameFlowState::Intro) && keyboardIntent) {
            flow.RequestPlaying();
        }
        const float speed = walkGrid->GetGridFrame().cellSize * 2.75F * moveSpeedScale;
        const float arrive = walkGrid->GetGridFrame().cellSize * 0.12F;
        if (useKeyboardDrive && keyboardIntent) {
            const float len = std::sqrt(moveX * moveX + moveY * moveY);
            moveX /= len;
            moveY /= len;
            playerRb->SetVelocity({moveX * speed, moveY * speed});
        } else if (!useKeyboardDrive && playerNav != nullptr &&
                   ApplyGridNavAgent2DRigidbodyMotion(
                           *playerNav,
                           *playerTr,
                           *playerRb,
                           speed,
                           arrive,
                           timing.deltaTimeSeconds)) {
            // path velocity applied
        } else if (!keyboardIntent && !useKeyboardDrive) {
            playerRb->SetVelocity(Vector2::Zero);
        } else if (!keyboardIntent) {
            playerRb->SetVelocity(Vector2::Zero);
        }
    } else if (playerRb != nullptr) {
        playerRb->SetVelocity(Vector2::Zero);
    }

    if (canMove) {
        physics.Simulate2D(world, timing);
        if (fogOfWar != nullptr && walkGrid != nullptr && playerObject != nullptr) {
            const Vector3 playerPos = playerObject->GetWorldMatrix().TransformPoint(Vector3::Zero);
            fogOfWar->RevealAtWorldPosition({playerPos.x, playerPos.y}, walkGrid->GetGridFrame());
        }
        TickGemPickupDissolves();
    }
    CorrectPlayerAgainstBlockedGrid();
    TryCollectNearbyGems(world);
    TryReachGoal();
    SyncCameraToPlayer(levelRoot != nullptr ? levelRoot->GetComponent<TilemapComponent>() : nullptr);
    helpHud.Update(timing, context);
    RefreshHud();
}

void GameFlow2DProductPathDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Vector3 pr{};
    Vector3 pu{};
    camera.BillboardBasisWorld(pr, pu);
    SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.55F, 0.62F, 0.72F}.Normalized(),
            Vector3{0.96F, 0.98F, 1.0F},
            0.65F,
            Vector3{0.08F, 0.10F, 0.14F},
            true,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderThenWorldY);

    SceneRenderParams* sceneParams = nullptr;
    if (context.TryGetMutableSceneRenderParams(sceneParams) && sceneParams != nullptr) {
        helpHud.PatchSceneRenderParams(*sceneParams, world);
        if (!useGpuMinimap) {
            RebuildMinimapIfNeeded();
        }
        if (minimapTexture && walkGrid != nullptr && playerTr != nullptr) {
            const Vector3 p = playerTr->GetLocalTransform().translation;
            PatchScene2DMinimapHud(
                    *sceneParams,
                    world,
                    minimapTexture,
                    walkGrid->GetGridFrame(),
                    {p.x, p.y},
                    static_cast<float>(fbW),
                    static_cast<float>(fbH),
                    useGpuMinimap);
        }
    }
}

}  // namespace Spark
