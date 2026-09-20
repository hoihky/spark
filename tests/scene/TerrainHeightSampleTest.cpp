#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/mesh/TerrainGeneratorSettings.hpp"

namespace {

Spark::TerrainComponent& AddFlatIslandTerrain(Spark::GameObject& object) {
    Spark::TerrainGeneratorSettings settings{};
    settings.subdivX = 32;
    settings.subdivZ = 32;
    settings.halfExtentX = 48.0F;
    settings.halfExtentZ = 48.0F;
    settings.heightScale = 0.0F;
    settings.noiseScale = 0.05F;
    settings.octaves = 1;
    Spark::TerrainComponent* terrain = object.AddComponent<Spark::TerrainComponent>(settings);
    terrain->ResetHeightsToProcedural(object);
    terrain->ApplyIslandFalloff(object, 12.0F, 28.0F, -5.0F);
    return *terrain;
}

}  // namespace

TEST(TerrainHeightSampleTest, IslandCenterIsAboveSubmergedDepth) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>();
    const Spark::TerrainComponent& terrain = AddFlatIslandTerrain(*object);

    float centerY = 0.0F;
    EXPECT_TRUE(terrain.TrySampleHeightWorld(*object, 0.0F, 0.0F, centerY));
    EXPECT_GT(centerY, -4.0F);
}

TEST(TerrainHeightSampleTest, FarShoreMatchesSubmergedDepth) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>();
    const Spark::TerrainComponent& terrain = AddFlatIslandTerrain(*object);

    float farY = 0.0F;
    EXPECT_TRUE(terrain.TrySampleHeightWorld(*object, 44.0F, 0.0F, farY));
    EXPECT_NEAR(farY, -5.0F, 0.25F);
}

TEST(TerrainHeightSampleTest, OutsideExtentReturnsFalse) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>();
    const Spark::TerrainComponent& terrain = AddFlatIslandTerrain(*object);

    float y = 0.0F;
    EXPECT_FALSE(terrain.TrySampleHeightWorld(*object, 200.0F, 0.0F, y));
}
