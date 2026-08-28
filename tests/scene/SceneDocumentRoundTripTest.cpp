#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/Tileset.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {

Spark::GameObject* FindByName(Spark::GameWorld& world, const char* name) {
    Spark::GameObject* found = nullptr;
    world.ForEachGameObject([&](Spark::GameObject* object) {
        if (object != nullptr && object->GetName() == Spark::Utf8String(name)) {
            found = object;
        }
    });
    return found;
}

const Spark::GameObject* FindByName(const Spark::GameWorld& world, const char* name) {
    const Spark::GameObject* found = nullptr;
    world.ForEachGameObject([&](Spark::GameObject* object) {
        if (object != nullptr && object->GetName() == Spark::Utf8String(name)) {
            found = object;
        }
    });
    return found;
}

void ExpectVec3Near(const Spark::Vector3& actual, const Spark::Vector3& expected, const float eps = 1e-4F) {
    EXPECT_NEAR(actual.x, expected.x, eps);
    EXPECT_NEAR(actual.y, expected.y, eps);
    EXPECT_NEAR(actual.z, expected.z, eps);
}

void BuildGameplayScene(Spark::GameWorld& world) {
    Spark::GameObject* root = world.CreateGameObject();
    root->GetName() = Spark::Utf8String("LevelRoot");
    root->AddComponent<Spark::TransformComponent>();

    Spark::GameObject* anchor = world.CreateGameObject();
    anchor->GetName() = Spark::Utf8String("Anchor");
    anchor->AddComponent<Spark::TransformComponent>()->SetTranslation({2.0F, 0.0F, 0.0F});
    world.SetParent(anchor, root);

    Spark::GameObject* platform = world.CreateGameObject();
    platform->GetName() = Spark::Utf8String("Platform");
    platform->AddComponent<Spark::TransformComponent>()->SetTranslation({-1.0F, 0.5F, 0.0F});
    Spark::Rigidbody2DComponent* rigidbody = platform->AddComponent<Spark::Rigidbody2DComponent>();
    rigidbody->SetGravityScale(0.25F);
    rigidbody->SetVelocity({0.8F, -0.2F});
    Spark::BoxCollider2DComponent* box = platform->AddComponent<Spark::BoxCollider2DComponent>();
    box->SetHalfExtents({1.0F, 0.25F});
    box->SetCategoryBits(0x2);
    Spark::DistanceJoint2DComponent* joint = platform->AddComponent<Spark::DistanceJoint2DComponent>();
    joint->SetConnectedBody(anchor);
    joint->SetRestLength(2.5F);
    world.SetParent(platform, root);

    Spark::GameObject* tilemapObject = world.CreateGameObject();
    tilemapObject->GetName() = Spark::Utf8String("GroundTilemap");
    tilemapObject->AddComponent<Spark::TransformComponent>();
    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/tilemap_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas = world.RegisterTexture(
            Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/tilemap_atlas");
    Spark::TilemapComponent* tilemap = tilemapObject->AddComponent<Spark::TilemapComponent>(
            Spark::MakeShared<Spark::Tileset>(atlas, 8U, 8U), 4U, 4U, 1.0F, 0);
    tilemap->SetTile(0U, 1U, 1U, 2U);
    tilemap->SetTile(0U, 2U, 2U, 4U);
    tilemapObject->AddComponent<Spark::TilemapCollider2DComponent>()->SetCategoryBits(0x8);
    Spark::TilemapGameplayGridComponent* grid = tilemapObject->AddComponent<Spark::TilemapGameplayGridComponent>();
    grid->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    grid->SetAutoRebake(true);
    world.SetParent(tilemapObject, root);

    Spark::GameObject* spawn = world.CreateGameObject();
    spawn->GetName() = Spark::Utf8String("PlayerSpawn");
    spawn->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 1.0F, 0.0F});
    Spark::SpawnPointComponent* spawnPoint = spawn->AddComponent<Spark::SpawnPointComponent>();
    spawnPoint->SetSpawnName("Player");
    spawnPoint->SetDefaultPrefabPath("prefabs/crate.sparkscene");

    Spark::GameObject* hud = world.CreateGameObject();
    hud->GetName() = Spark::Utf8String("GameHud");
    Spark::UiCanvasComponent* canvas = hud->AddComponent<Spark::UiCanvasComponent>();
    canvas->SetSortOrder(5);
    canvas->SetModalInputCapture(true);
    Spark::Ui::SparkUiControlsFactory factory{};
    Spark::Ui::PanelDesc panelDesc{};
    panelDesc.id = Spark::Utf8String("hud_panel");
    panelDesc.title = Spark::Utf8String("Stats");
    Spark::UniquePtr<Spark::Ui::IPanel> panel = factory.CreatePanel(panelDesc);
    Spark::Ui::ButtonDesc buttonDesc{};
    buttonDesc.id = Spark::Utf8String("restart_btn");
    buttonDesc.label = Spark::Utf8String("Restart");
    panel->AddChild(Spark::UniquePtr<Spark::Ui::IUiElement>(
            static_cast<Spark::Ui::IUiElement*>(factory.CreateButton(buttonDesc).Release())));
    canvas->AdoptRoot(Spark::UniquePtr<Spark::Ui::IUiElement>(
            static_cast<Spark::Ui::IUiElement*>(panel.Release())));

    Spark::GameObject* enemy = world.CreateGameObject();
    enemy->GetName() = Spark::Utf8String("EnemyBrain");
    Spark::AiAgentComponent* agent = enemy->AddComponent<Spark::AiAgentComponent>();
    agent->SetMaxSpeed(4.0F);
    agent->SetGoapEnabled(true);
    enemy->AddComponent<Spark::PerceptionSensorComponent>()->SetSightRadius(12.0F);
}

void VerifyGameplayScene(const Spark::GameWorld& world) {
    const Spark::GameObject* root = FindByName(world, "LevelRoot");
    ASSERT_NE(root, nullptr);

    const Spark::GameObject* anchor = FindByName(world, "Anchor");
    const Spark::GameObject* platform = FindByName(world, "Platform");
    ASSERT_NE(anchor, nullptr);
    ASSERT_NE(platform, nullptr);
    ASSERT_EQ(platform->GetParent(), root);
    ASSERT_EQ(anchor->GetParent(), root);

    ExpectVec3Near(
            anchor->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation,
            {2.0F, 0.0F, 0.0F});
    ExpectVec3Near(
            platform->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation,
            {-1.0F, 0.5F, 0.0F});

    const Spark::Rigidbody2DComponent* restoredRb = platform->GetComponent<Spark::Rigidbody2DComponent>();
    ASSERT_NE(restoredRb, nullptr);
    EXPECT_FLOAT_EQ(restoredRb->GetGravityScale(), 0.25F);
    EXPECT_FLOAT_EQ(restoredRb->GetVelocity().x, 0.8F);

    const Spark::BoxCollider2DComponent* restoredBox = platform->GetComponent<Spark::BoxCollider2DComponent>();
    ASSERT_NE(restoredBox, nullptr);
    EXPECT_EQ(restoredBox->GetCategoryBits(), 0x2);

    const Spark::DistanceJoint2DComponent* restoredJoint = platform->GetComponent<Spark::DistanceJoint2DComponent>();
    ASSERT_NE(restoredJoint, nullptr);
    EXPECT_EQ(restoredJoint->GetConnectedBody(), anchor);
    EXPECT_FLOAT_EQ(restoredJoint->GetRestLength(), 2.5F);

    const Spark::GameObject* tilemapObject = FindByName(world, "GroundTilemap");
    ASSERT_NE(tilemapObject, nullptr);
    ASSERT_EQ(tilemapObject->GetParent(), root);
    const Spark::TilemapComponent* restoredTilemap = tilemapObject->GetComponent<Spark::TilemapComponent>();
    ASSERT_NE(restoredTilemap, nullptr);
    EXPECT_EQ(restoredTilemap->GetMapWidth(), 4U);
    EXPECT_EQ(restoredTilemap->GetTileCell(0U, 1U, 1U).tileId, 2U);
    EXPECT_EQ(restoredTilemap->GetTileCell(0U, 2U, 2U).tileId, 4U);
    const Spark::TilemapCollider2DComponent* restoredCollider =
            tilemapObject->GetComponent<Spark::TilemapCollider2DComponent>();
    ASSERT_NE(restoredCollider, nullptr);
    EXPECT_EQ(restoredCollider->GetCategoryBits(), 0x8);
    const Spark::TilemapGameplayGridComponent* restoredGrid =
            tilemapObject->GetComponent<Spark::TilemapGameplayGridComponent>();
    ASSERT_NE(restoredGrid, nullptr);
    EXPECT_EQ(restoredGrid->GetWalkRule(), Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    EXPECT_TRUE(restoredGrid->GetAutoRebake());

    const Spark::GameObject* spawn = FindByName(world, "PlayerSpawn");
    ASSERT_NE(spawn, nullptr);
    ExpectVec3Near(
            spawn->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation,
            {0.0F, 1.0F, 0.0F});
    const Spark::SpawnPointComponent* restoredSpawn = spawn->GetComponent<Spark::SpawnPointComponent>();
    ASSERT_NE(restoredSpawn, nullptr);
    EXPECT_STREQ(restoredSpawn->GetSpawnName().CStr(), "Player");
    EXPECT_STREQ(restoredSpawn->GetDefaultPrefabPath().CStr(), "prefabs/crate.sparkscene");

    const Spark::GameObject* hud = FindByName(world, "GameHud");
    ASSERT_NE(hud, nullptr);
    const Spark::UiCanvasComponent* restoredCanvas = hud->GetComponent<Spark::UiCanvasComponent>();
    ASSERT_NE(restoredCanvas, nullptr);
    EXPECT_EQ(restoredCanvas->GetSortOrder(), 5);
    EXPECT_TRUE(restoredCanvas->GetModalInputCapture());
    ASSERT_NE(restoredCanvas->GetRoot(), nullptr);
    const Spark::Ui::SparkPanel* restoredPanel =
            dynamic_cast<const Spark::Ui::SparkPanel*>(restoredCanvas->GetRoot());
    ASSERT_NE(restoredPanel, nullptr);
    EXPECT_STREQ(restoredPanel->ExportDesc().title.CStr(), "Stats");
    ASSERT_EQ(restoredPanel->GetChildren().GetSize(), 1U);

    const Spark::GameObject* enemy = FindByName(world, "EnemyBrain");
    ASSERT_NE(enemy, nullptr);
    const Spark::AiAgentComponent* restoredAgent = enemy->GetComponent<Spark::AiAgentComponent>();
    ASSERT_NE(restoredAgent, nullptr);
    EXPECT_FLOAT_EQ(restoredAgent->GetMaxSpeed(), 4.0F);
    EXPECT_TRUE(restoredAgent->IsGoapEnabled());
    const Spark::PerceptionSensorComponent* restoredSensor = enemy->GetComponent<Spark::PerceptionSensorComponent>();
    ASSERT_NE(restoredSensor, nullptr);
    EXPECT_FLOAT_EQ(restoredSensor->GetSightRadius(), 12.0F);
}

[[nodiscard]] Spark::SceneDocument CaptureAndSerializeRoundTrip(const Spark::GameWorld& source) {
    Spark::SceneSerializer serializer{};
    Spark::SceneCaptureContext captureCtx{};
    Spark::SceneDocument captured = serializer.Capture(source, captureCtx, {});
    EXPECT_GE(captured.entities.GetSize(), 6U);

    Spark::Utf8String serialized;
    EXPECT_TRUE(serializer.WriteToString(captured, serialized));
    EXPECT_NE(std::strstr(serialized.CStr(), Spark::SceneDocument::kMagic), nullptr);

    Spark::SceneDocument parsed{};
    Spark::SceneDeserializer deserializer{};
    EXPECT_TRUE(deserializer.ReadFromString(serialized.CStr(), parsed));
    EXPECT_EQ(parsed.entities.GetSize(), captured.entities.GetSize());
    return parsed;
}

}  // namespace

TEST(SceneDocumentRoundTripTest, ProgrammaticGameplaySceneRoundTrip) {
    Spark::GameWorld sourceWorld{};
    BuildGameplayScene(sourceWorld);
    const std::size_t sourceCount = sourceWorld.GetGameObjectCount();

    Spark::SceneDocument document = CaptureAndSerializeRoundTrip(sourceWorld);

    Spark::GameWorld restoredWorld{};
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = "assets";
    applyCtx.assetLoader = &restoredWorld.GetAssetLoader();
    Spark::SceneDeserializer deserializer{};
    ASSERT_TRUE(deserializer.Apply(document, restoredWorld, applyCtx));

    EXPECT_EQ(restoredWorld.GetGameObjectCount(), document.entities.GetSize());
    EXPECT_EQ(restoredWorld.GetGameObjectCount(), sourceCount);
    VerifyGameplayScene(restoredWorld);
}

TEST(SceneDocumentRoundTripTest, BundledArenaSceneFileRoundTrip) {
    Spark::Utf8String scenePath(SPARK_ASSETS_DIR);
    scenePath.AppendUtf8("/scenes/arena.sparkscene");
    struct stat fileStat {};
    if (stat(scenePath.CStr(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        GTEST_SKIP() << "arena.sparkscene not available";
    }

    Spark::SceneDeserializer deserializer{};
    Spark::SceneDocument original{};
    ASSERT_TRUE(deserializer.ReadFromFile(scenePath.CStr(), original));
    EXPECT_EQ(original.entities.GetSize(), 5U);
    EXPECT_STREQ(original.header.name.CStr(), "Arena");

    Spark::GameWorld firstPass{};
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_ASSETS_DIR;
    applyCtx.assetLoader = &firstPass.GetAssetLoader();
    ASSERT_TRUE(deserializer.Apply(original, firstPass, applyCtx));
    EXPECT_EQ(firstPass.GetGameObjectCount(), 5U);

    const Spark::GameObject* spawn = FindByName(firstPass, "PlayerSpawn");
    ASSERT_NE(spawn, nullptr);
    const Spark::SpawnPointComponent* spawnPoint = spawn->GetComponent<Spark::SpawnPointComponent>();
    ASSERT_NE(spawnPoint, nullptr);
    EXPECT_STREQ(spawnPoint->GetSpawnName().CStr(), "Player");

    const Spark::GameObject* crate = FindByName(firstPass, "CrateA");
    ASSERT_NE(crate, nullptr);
    ExpectVec3Near(
            crate->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation,
            {-3.0F, 0.425F, 2.0F});

    Spark::SceneSerializer serializer{};
    Spark::SceneDocument recaptured = serializer.Capture(firstPass, {}, {});
    EXPECT_EQ(recaptured.entities.GetSize(), original.entities.GetSize());

    Spark::Utf8String serialized{};
    ASSERT_TRUE(serializer.WriteToString(recaptured, serialized));

    Spark::SceneDocument reparsed{};
    ASSERT_TRUE(deserializer.ReadFromString(serialized.CStr(), reparsed));
    EXPECT_EQ(reparsed.entities.GetSize(), original.entities.GetSize());

    Spark::GameWorld secondPass{};
    Spark::SceneApplyContext secondApplyCtx{};
    secondApplyCtx.assetsRoot = SPARK_ASSETS_DIR;
    secondApplyCtx.assetLoader = &secondPass.GetAssetLoader();
    ASSERT_TRUE(deserializer.Apply(reparsed, secondPass, secondApplyCtx));
    EXPECT_EQ(secondPass.GetGameObjectCount(), original.entities.GetSize());
    EXPECT_NE(FindByName(secondPass, "WarmLight"), nullptr);
    EXPECT_NE(FindByName(secondPass, "CrateB"), nullptr);
    EXPECT_NE(FindByName(secondPass, "Barrel"), nullptr);
}
