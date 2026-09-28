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
#include "spark/scene/tilemap/TilemapLayer.hpp"
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
#include "spark/scene/tilemap/KenneyTinyDungeonGameplay.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/ai/path/GridPathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

struct ProductPathSpawnBindings {
    GameFlow2DProductPathDemo* demo = nullptr;
    SharedPtr<Texture2D> gemTexture{};
    GameObject* flowObject = nullptr;
    GameObject** goalObject = nullptr;
    int gemsRequired = 3;
};

ProductPathSpawnBindings g_bindings{};

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
    GameObject* gem = world.CreateGameObject();
    gem->GetName() = Utf8String("P0Gem");
    TransformComponent* tr = gem->AddComponent<TransformComponent>();
    tr->SetTranslation({center.x, center.y, 0.06F});
    const float gemScale = std::max(frame.cellSize * 0.55F, 0.5F);
    tr->SetScale({gemScale, gemScale, 1.0F});
    gem->AddComponent<SpriteComponent>(
            g_bindings.gemTexture,
            Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            80);
    if (TriggerVolume2DComponent* trigger =
                gem->AddComponent<TriggerVolume2DComponent>(TriggerVolume2DShape::Circle, Vector2{0.5F, 0.5F}, Vector2::Zero)) {
        trigger->SetRadius(std::max(frame.cellSize * 0.35F, 0.35F));
    }
    gem->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
    auto* pickup = gem->AddComponent<PickupComponent>();
    pickup->SetItemId("gem");
    pickup->SetOnCollected([gem](GameObject& /*collector*/, const char*, int) {
        if (g_bindings.demo != nullptr) {
            g_bindings.demo->OnGemPickedUp();
        }
        if (gem != nullptr) {
            gem->SetActive(false);
        }
    });
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

[[nodiscard]] float CellDistanceSqToWorld(
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& cell,
        const Vector2& worldXY) noexcept {
    const Vector2 center = frame.CellCenterToWorldXY(cell);
    const float dx = center.x - worldXY.x;
    const float dy = center.y - worldXY.y;
    return dx * dx + dy * dy;
}

void SortCellsByDistanceFrom(
        Array<GridPathfinder::Cell>& cells,
        const TilemapGridFrame& frame,
        const Vector2& worldXY) noexcept {
    for (std::size_t i = 0; i + 1U < cells.GetSize(); ++i) {
        for (std::size_t j = i + 1U; j < cells.GetSize(); ++j) {
            const float di = CellDistanceSqToWorld(frame, cells[i], worldXY);
            const float dj = CellDistanceSqToWorld(frame, cells[j], worldXY);
            if (dj < di) {
                const GridPathfinder::Cell tmp = cells[i];
                cells[i] = cells[j];
                cells[j] = tmp;
            }
        }
    }
}

[[nodiscard]] bool TryPlacePlayerOnSandCell(
        TransformComponent& playerTr,
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& cell,
        Vector3& lastValidPos) noexcept {
    if (!frame.IsCellInBounds(cell) ||
            !IsKenneySandMapCell(tilemap, dungeonLayerIndex, cell.x, cell.y)) {
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
    const TilemapComponent* tilemap = levelRoot->GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        return;
    }
    if (playerRb != nullptr) {
        playerRb->SetVelocity(Vector2::Zero);
    }
    const std::uint32_t dungeonLayer = FindKenneyDungeonLayerIndex(*tilemap);
    (void)TryPlacePlayerOnSandCell(
            *playerTr,
            *tilemap,
            dungeonLayer,
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
    TilemapComponent* tilemap = levelRoot.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        return;
    }

    walkGrid->RebakeIfNeeded(levelRoot);

    const TilemapGridFrame& frame = walkGrid->GetGridFrame();
    const std::uint32_t dungeonLayer = FindKenneyDungeonLayerIndex(*tilemap);

    const std::size_t minPlayableCells = static_cast<std::size_t>(gemsRequired + 2U);
    GridPathfinder::Cell startCell{};
    if (!PickKenneySandSpawnCell(
                *tilemap,
                dungeonLayer,
                frame,
                spawnHintWorld,
                minPlayableCells,
                startCell)) {
        return;
    }

    playerSpawnCell = startCell;
    playerSpawnResolved = true;

    Array<GridPathfinder::Cell> reachable{};
    CollectReachableKenneySandCells(*tilemap, dungeonLayer, startCell, reachable);
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
    SortCellsByDistanceFrom(placements, frame, spawnWorld);

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

void GameFlow2DProductPathDemo::ConfigureLevelLayers(GameObject& root) noexcept {
    TilemapComponent* tilemap = root.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        return;
    }
    for (std::uint32_t layerIndex = 0U; layerIndex < tilemap->GetLayerCount(); ++layerIndex) {
        TilemapLayer& layer = tilemap->GetLayer(layerIndex);
        const char* name = layer.name.CStr();
        if (name == nullptr) {
            continue;
        }
        if (std::strcmp(name, "Carts") == 0) {
            layer.contributeCollision = false;
            layer.contributeGameplayGrid = false;
        } else if (std::strcmp(name, "Objects") == 0) {
            layer.contributeGameplayGrid = false;
        }
    }
}

void GameFlow2DProductPathDemo::CorrectPlayerAgainstBlockedGrid() noexcept {
    if (playerTr == nullptr || playerRb == nullptr || playerCollider == nullptr || walkGrid == nullptr) {
        return;
    }
    const TilemapGameplayGrid& grid = walkGrid->GetGrid();
    const TilemapGridFrame& frame = walkGrid->GetGridFrame();

    const TilemapComponent* tilemap = levelRoot != nullptr ? levelRoot->GetComponent<TilemapComponent>() : nullptr;
    const std::uint32_t dungeonLayer =
            tilemap != nullptr ? FindKenneyDungeonLayerIndex(*tilemap) : 0U;
    auto isBlockedAt = [&](const float worldX, const float worldY) noexcept {
        const GridPathfinder::Cell cell = frame.WorldXYToCell({worldX, worldY});
        if (tilemap != nullptr && cell.x >= 0 && cell.y >= 0) {
            return !IsKenneySandMapCell(*tilemap, dungeonLayer, cell.x, cell.y);
        }
        return !grid.IsWalkable(cell.x, cell.y);
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
    saveStatus = Utf8String("Loaded (F7)");
    RefreshHud();
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

    ApplyKenneyTinyDungeonGameplayToTilemap(*levelRoot);
    ConfigureLevelLayers(*levelRoot);

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
    g_bindings.gemTexture = MakeGemTexture();
    g_bindings.flowObject = flowObject;
    g_bindings.goalObject = &goalObject;
    g_bindings.gemsRequired = gemsRequired;
    goalObject = nullptr;
    RegisterSpawnHandlers();

    if (TilemapObjectSpawnComponent* spawn = levelRoot->GetComponent<TilemapObjectSpawnComponent>()) {
        spawn->SetSpawnOnAttach(false);
        spawn->RespawnAll(*levelRoot, world);
    }

    physics.GetQueries2D().RebuildStatics(world);

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

void GameFlow2DProductPathDemo::OnGemPickedUp() noexcept {
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
    persistent = GameFlowPersistentData{};
    flow.LoadProgressFromDisk(GameSave::DefaultSlotPath("p0_demo").CStr());

    sceneManager = MakeUnique<SceneManager>(world);
    loadSession = MakeUnique<SceneLoadSession>(*sceneManager);

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
    roots.Track(playerObject);

    RebuildAuthoredLevel(world);

    helpHud.Mount(world, "2D P0 product path");
    helpHud.SetControlHints(
            "WASD move | Wheel/+/- zoom | Enter start | P pause | R reload | F5 save | F7 load");
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
    sceneManager.Reset();
    flowObject = nullptr;
    gameState = nullptr;
    playerObject = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    playerCollider = nullptr;
    playerSpawnResolved = false;
    saveStatus.Clear();
    roots.DestroyAll(world);
    physics = PhysicsSubsystem{};
    gemsCollected = 0;
    levelStatus.Clear();
}

void GameFlow2DProductPathDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    const float dt = timing.deltaTimeSeconds;
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
    if (canMove && playerRb != nullptr && walkGrid != nullptr) {
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
        if (gameState->IsState(GameFlowState::Intro) && (std::abs(moveX) > 0.01F || std::abs(moveY) > 0.01F)) {
            flow.RequestPlaying();
        }
        const float speed = walkGrid->GetGridFrame().cellSize * 2.75F;
        if (std::abs(moveX) > 0.01F || std::abs(moveY) > 0.01F) {
            const float len = std::sqrt(moveX * moveX + moveY * moveY);
            moveX /= len;
            moveY /= len;
        }
        playerRb->SetVelocity({moveX * speed, moveY * speed});
    } else if (playerRb != nullptr) {
        playerRb->SetVelocity(Vector2::Zero);
    }

    physics.Simulate2D(world, timing);
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
    }
}

}  // namespace Spark
