#include <gtest/gtest.h>

#include <sys/stat.h>

#include "spark/config.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/tilemap/TilemapGameplayPlacement.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace {

const Spark::GameObject* FindByName(const Spark::GameWorld& world, const char* name) {
    const Spark::GameObject* found = nullptr;
    world.ForEachGameObject([&](const Spark::GameObject* object) {
        if (found == nullptr && object != nullptr && object->GetName() == Spark::Utf8String(name)) {
            found = object;
        }
    });
    return found;
}

}  // namespace

TEST(TilemapGameplayPlacementTest, P0SpawnUsesLargestWalkableRegionNearCourtyard) {
    Spark::Utf8String scenePath(SPARK_ASSETS_DIR);
    scenePath.AppendUtf8("/scenes/platformer_level.sparkscene");
    struct stat fileStat {};
    if (stat(scenePath.CStr(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        GTEST_SKIP() << "platformer_level.sparkscene not available";
    }

    Spark::SceneDeserializer deserializer{};
    Spark::SceneDocument document{};
    ASSERT_TRUE(deserializer.ReadFromFile(scenePath.CStr(), document));

    Spark::GameWorld world{};
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_ASSETS_DIR;
    applyCtx.assetLoader = &world.GetAssetLoader();
    ASSERT_TRUE(deserializer.Apply(document, world, applyCtx));

    Spark::GameObject* level = const_cast<Spark::GameObject*>(FindByName(world, "Level"));
    ASSERT_NE(level, nullptr);
    Spark::TilemapComponent* tilemap = level->GetComponent<Spark::TilemapComponent>();
    ASSERT_NE(tilemap, nullptr);

    Spark::TilemapMapSourceComponent* mapSource = level->GetComponent<Spark::TilemapMapSourceComponent>();
    if (mapSource == nullptr) {
        GTEST_SKIP() << "Level has no TilemapMapSourceComponent";
    }
    mapSource->SetImportOnAttach(false);
    ASSERT_TRUE(mapSource->ImportNow(*level, world));

    Spark::ApplyDefaultGameplayLayerFlags(*tilemap);

    auto* walkGrid = level->GetComponent<Spark::TilemapGameplayGridComponent>();
    if (walkGrid == nullptr) {
        walkGrid = level->AddComponent<Spark::TilemapGameplayGridComponent>();
    }
    walkGrid->SetWalkRule(Spark::TilemapGameplayWalkRule::CollisionAligned);
    walkGrid->RequestRebake();
    walkGrid->RebakeIfNeeded(*level);

    const Spark::IGridWalkability& walk = walkGrid->GetWalkability();
    const Spark::TilemapGridFrame& frame = walkGrid->GetGridFrame();

    Spark::GridPathfinder::Cell spawn{};
    const Spark::Vector2 courtyardHint{15.5F, 9.5F};
    ASSERT_TRUE(Spark::PickSpawnInLargestWalkableRegion(walk, frame, courtyardHint, 5U, spawn));

    EXPECT_TRUE(Spark::IsWalkableMapCell(walk, frame, spawn.x, spawn.y));
    EXPECT_TRUE(Spark::IsWalkableMapCell(walk, frame, 15, 9));

    const float nearCourtyardDx = static_cast<float>(spawn.x - 15);
    const float nearCourtyardDy = static_cast<float>(spawn.y - 9);
    EXPECT_LT(nearCourtyardDx * nearCourtyardDx + nearCourtyardDy * nearCourtyardDy, 9.0F);

    const float roofPatchDx = static_cast<float>(spawn.x - 8);
    const float roofPatchDy = static_cast<float>(spawn.y - 14);
    EXPECT_GT(roofPatchDx * roofPatchDx + roofPatchDy * roofPatchDy, 16.0F);
}
