#include "spark/demo/platformer2d/Platformer2DLevelPipeline.hpp"

#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/demo/platformer2d/Platformer2DConfig.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/editor/SceneLevelLoader.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

namespace {

GameObject* FindLevelRoot(GameWorld& world, const SceneInstanceId instanceId) noexcept {
    GameObject* found = nullptr;
    world.ForEachGameObject([&](GameObject* object) {
        if (found != nullptr || object == nullptr || object->GetSceneInstanceId() != instanceId) {
            return;
        }
        if (object->GetComponent<TilemapMapSourceComponent>() != nullptr) {
            found = object;
        }
    });
    return found;
}

void SeedMarkerAtWorld(
        TilemapObjectLayerComponent& objects,
        const std::uint32_t layerIndex,
        const TilemapGridFrame& frame,
        const char* typeId,
        const float worldX,
        const float worldY,
        const char* name = nullptr) noexcept {
    const GridPathfinder::Cell cell = frame.WorldXYToCell({worldX, worldY});
    TilemapObjectMarker marker{};
    marker.typeId = Utf8String(typeId);
    if (name != nullptr) {
        marker.name = Utf8String(name);
    }
    marker.cellX = cell.x;
    marker.cellY = cell.y;
    marker.offsetX = 0.5F;
    marker.offsetY = 0.5F;
    marker.mode = TilemapObjectMarkerMode::Runtime;
    objects.AddMarker(layerIndex, marker);
}

}  // namespace

void Platformer2DLevelPipeline::SeedLegacyPlatformerMarkers(GameObject& levelRoot) noexcept {
    TilemapObjectLayerComponent* objects = levelRoot.GetComponent<TilemapObjectLayerComponent>();
    const TilemapComponent* tilemap = levelRoot.GetComponent<TilemapComponent>();
    if (objects == nullptr || tilemap == nullptr) {
        return;
    }
    if (objects->GetLayerCount() == 0U) {
        objects->AddObjectLayer("Gameplay");
    }
    const std::uint32_t layerIndex = 0U;
    objects->ClearMarkers(layerIndex);

    const TilemapGridFrame frame = TilemapGridFrame::FromTilemapObject(levelRoot, *tilemap);

    for (int gi = 0; gi < Platformer2DDemo::kGemCount; ++gi) {
        SeedMarkerAtWorld(
                *objects,
                layerIndex,
                frame,
                "gem",
                Platformer2DDemo::kGemSpawns[static_cast<std::size_t>(gi)][0],
                Platformer2DDemo::kGemSpawns[static_cast<std::size_t>(gi)][1]);
    }
    for (int ei = 0; ei < Spark::Platformer2D::Config::kEnemyCount; ++ei) {
        SeedMarkerAtWorld(
                *objects,
                layerIndex,
                frame,
                "enemy",
                Spark::Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][0],
                Spark::Platformer2D::Config::kEnemySpawns[static_cast<std::size_t>(ei)][1],
                "PlatEnemy");
    }
    SeedMarkerAtWorld(
            *objects,
            layerIndex,
            frame,
            "goal",
            Spark::Platformer2D::Config::kGoalCenterX,
            Spark::Platformer2D::Config::kGoalCenterY,
            "PlatGoal");
}

void Platformer2DLevelPipeline::EnsureGameplayAttachments(GameObject& levelRoot) noexcept {
    if (levelRoot.GetComponent<TilemapCollider2DComponent>() == nullptr) {
        levelRoot.AddComponent<TilemapCollider2DComponent>();
    }
    if (levelRoot.GetComponent<TilemapObjectLayerComponent>() == nullptr) {
        levelRoot.AddComponent<TilemapObjectLayerComponent>();
    }
    if (levelRoot.GetComponent<TilemapObjectSpawnComponent>() == nullptr) {
        levelRoot.AddComponent<TilemapObjectSpawnComponent>();
    }
}

Platformer2DLevelLoadResult Platformer2DLevelPipeline::TryLoad(
        GameWorld& world,
        SceneLoadSession& session,
        DemoRootCollection& roots,
        const char* scenePath) noexcept {
    Platformer2DLevelLoadResult result{};
    if (scenePath == nullptr || scenePath[0] == '\0') {
        result.message = Utf8String("Level scene path is empty.");
        return result;
    }
    if (!ScenePathResolver::FileExists(scenePath)) {
        result.message = Utf8String("Level scene file not found.");
        return result;
    }

    SceneLevelLoader loader(session);
    const SceneLevelLoadResult loaded = loader.Load(world, scenePath, &FindLevelRoot, "Player");
    if (!loaded.success || loaded.primaryEntity == nullptr) {
        result.message = loaded.message.IsEmpty() ? Utf8String("SceneLevelLoader failed.") : loaded.message;
        return result;
    }

    result.instanceId = loaded.instanceId;
    result.levelRoot = loaded.primaryEntity;
    result.playerSpawnPose = loaded.spawnPose;
    roots.Track(result.levelRoot);

    EnsureGameplayAttachments(*result.levelRoot);

    if (TilemapObjectSpawnComponent* spawn = result.levelRoot->GetComponent<TilemapObjectSpawnComponent>()) {
        spawn->SetSpawnOnAttach(false);
    }

    result.success = true;
    result.message = Utf8String("Authored level loaded.");
    return result;
}

}  // namespace Spark
