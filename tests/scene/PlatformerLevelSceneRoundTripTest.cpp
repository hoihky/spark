#include <gtest/gtest.h>

#include <sys/stat.h>

#include "spark/config.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

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

TEST(PlatformerLevelSceneRoundTripTest, BundledPlatformerLevelPreservesMapSourceAndSpawn) {
    Spark::Utf8String scenePath(SPARK_ASSETS_DIR);
    scenePath.AppendUtf8("/scenes/platformer_level.sparkscene");
    struct stat fileStat {};
    if (stat(scenePath.CStr(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        GTEST_SKIP() << "platformer_level.sparkscene not available";
    }

    Spark::SceneDeserializer deserializer{};
    Spark::SceneDocument original{};
    ASSERT_TRUE(deserializer.ReadFromFile(scenePath.CStr(), original));

    Spark::GameWorld world{};
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_ASSETS_DIR;
    applyCtx.assetLoader = &world.GetAssetLoader();
    ASSERT_TRUE(deserializer.Apply(original, world, applyCtx));

    const Spark::GameObject* level = FindByName(world, "Level");
    ASSERT_NE(level, nullptr);
    const Spark::TilemapMapSourceComponent* mapSource = level->GetComponent<Spark::TilemapMapSourceComponent>();
    ASSERT_NE(mapSource, nullptr);
    EXPECT_NE(std::strstr(mapSource->GetTmxPath().CStr(), "sampleMap.tmx"), nullptr);

    const Spark::GameObject* spawn = FindByName(world, "PlayerSpawn");
    ASSERT_NE(spawn, nullptr);
    const Spark::SpawnPointComponent* spawnPoint = spawn->GetComponent<Spark::SpawnPointComponent>();
    ASSERT_NE(spawnPoint, nullptr);
    EXPECT_STREQ(spawnPoint->GetSpawnName().CStr(), "Player");

    Spark::SceneSerializer serializer{};
    Spark::SceneDocument captured = serializer.Capture(world, {}, {});
    Spark::Utf8String serialized{};
    ASSERT_TRUE(serializer.WriteToString(captured, serialized));

    Spark::SceneDocument reparsed{};
    ASSERT_TRUE(deserializer.ReadFromString(serialized.CStr(), reparsed));
    EXPECT_EQ(reparsed.entities.GetSize(), original.entities.GetSize());

    const Spark::EntityRecord* levelEntity = nullptr;
    for (std::size_t i = 0; i < reparsed.entities.GetSize(); ++i) {
        if (reparsed.entities[i].name == Spark::Utf8String("Level")) {
            levelEntity = &reparsed.entities[i];
            break;
        }
    }
    ASSERT_NE(levelEntity, nullptr);
    bool foundMapSource = false;
    for (std::size_t ci = 0; ci < levelEntity->components.GetSize(); ++ci) {
        if (levelEntity->components[ci].kind == Spark::Utf8String("tilemap_map_source")) {
            foundMapSource = true;
            EXPECT_NE(std::strstr(levelEntity->components[ci].payload.CStr(), "sampleMap.tmx"), nullptr);
            break;
        }
    }
    EXPECT_TRUE(foundMapSource);
}
