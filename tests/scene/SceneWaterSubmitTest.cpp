#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace {

Spark::GameObject& AddWaterBody(Spark::GameWorld& world, float worldX) {
    Spark::GameObject* object = world.CreateGameObject();
    EXPECT_TRUE(object != nullptr);
    if (Spark::TransformComponent* transform = object->GetComponent<Spark::TransformComponent>()) {
        transform->SetTranslation({worldX, 0.0F, 0.0F});
    }
    Spark::WaterBodyComponent* water = object->AddComponent<Spark::WaterBodyComponent>();
    water->OnAttach(*object);
    return *object;
}

}  // namespace

TEST(SceneWaterSubmitTest, WaterBodyFillsWaterDrawsWithoutShadowParticipation) {
    Spark::GameWorld world{};
    AddWaterBody(world, 0.0F);

    Spark::SceneRenderParams params{};
    const auto findTex = [](const Spark::SharedPtr<Spark::Texture2D>&, Spark::Vector2*, Spark::Vector2*) -> std::int32_t {
        return -1;
    };

    Spark::SceneSubmitDetail::SubmitWaterBodiesFromWorld(
            world, Spark::Matrix4::Identity, {0.0F, 0.0F, 0.0F}, params, findTex, nullptr);

    EXPECT_EQ(params.waterDraws.GetSize(), 1U);
    EXPECT_EQ(params.waterDraws[0].item.shadowFlags, 0);
    EXPECT_FALSE(params.waterDraws[0].item.doubleSided);
    EXPECT_EQ(params.waterDraws[0].item.mesh, Spark::SceneMeshSlot::Custom);
}

TEST(SceneWaterSubmitTest, WaterDrawsSortFartherFirst) {
    Spark::GameWorld world{};
    AddWaterBody(world, 0.0F);
    AddWaterBody(world, 40.0F);

    Spark::SceneRenderParams params{};
    const auto findTex = [](const Spark::SharedPtr<Spark::Texture2D>&, Spark::Vector2*, Spark::Vector2*) -> std::int32_t {
        return -1;
    };

    Spark::SceneSubmitDetail::SubmitWaterBodiesFromWorld(
            world, Spark::Matrix4::Identity, {0.0F, 0.0F, 0.0F}, params, findTex, nullptr);

    EXPECT_EQ(params.waterDraws.GetSize(), 2U);
    EXPECT_GE(params.waterDraws[0].sortDepth, params.waterDraws[1].sortDepth);
}

TEST(SceneWaterSubmitTest, OpaquePartitionExcludesWaterMeshes) {
    Spark::GameWorld world{};
    AddWaterBody(world, 0.0F);

    Spark::Array<Spark::SceneDrawItem> drawList;
    Spark::SceneDrawItem cube{};
    cube.mesh = Spark::SceneMeshSlot::UnitCube;
    drawList.PushBack(cube);

    Spark::SceneRenderParams params{};
    Spark::PartitionSortedDrawItemsIntoSceneParams(drawList, params, {0.0F, 0.0F, 0.0F});

    const auto findTex = [](const Spark::SharedPtr<Spark::Texture2D>&, Spark::Vector2*, Spark::Vector2*) -> std::int32_t {
        return -1;
    };
    Spark::SceneSubmitDetail::SubmitWaterBodiesFromWorld(
            world, Spark::Matrix4::Identity, {0.0F, 0.0F, 0.0F}, params, findTex, nullptr);

    EXPECT_EQ(params.draws.GetSize(), 1U);
    EXPECT_EQ(params.waterDraws.GetSize(), 1U);
    EXPECT_EQ(params.draws[0].mesh, Spark::SceneMeshSlot::UnitCube);
}
