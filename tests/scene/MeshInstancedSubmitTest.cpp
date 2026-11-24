#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/RigidMeshInstancedBatchComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/MeshInstancedSceneCollector.hpp"

TEST(MeshInstancedSubmitTest, CollectorBuildsOpaqueInstancedBatch) {
    Spark::GameWorld world{};
    Spark::GameObject* root = world.CreateGameObject();
    root->AddComponent<Spark::MeshComponent>(Spark::SharedPtr<Spark::Mesh>{}, Spark::SceneMeshSlot::UnitCube, Spark::Vector3::One);
    auto* batch = root->AddComponent<Spark::RigidMeshInstancedBatchComponent>();
    batch->ConfigureGrid(4, 4, 1.0F);

    Spark::SceneRenderParams params{};
    const auto findTexture = [](const Spark::SharedPtr<Spark::Texture2D>&,
                                Spark::Vector2*,
                                Spark::Vector2*) -> std::int32_t { return -1; };

    Spark::MeshInstancedSceneCollector collector{};
    collector.CollectInto(world, params, findTexture);

    ASSERT_EQ(params.meshInstancedBatches.GetSize(), 1U);
    EXPECT_EQ(params.meshInstances.GetSize(), 16U);
    EXPECT_EQ(params.meshInstancedBatches[0].GetInstanceCount(), 16U);
    EXPECT_EQ(params.meshInstancedBatches[0].GetDrawTemplate().mesh, Spark::SceneMeshSlot::UnitCube);
    EXPECT_FLOAT_EQ(params.meshInstancedBatches[0].GetDrawTemplate().opacity, 1.0F);
}