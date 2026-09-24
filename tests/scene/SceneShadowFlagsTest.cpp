#include <gtest/gtest.h>

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/mesh/TerrainGeneratorSettings.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace {

constexpr std::int32_t kDefaultFlags = Spark::kSceneShadowCastAndReceive;

}  // namespace

TEST(SceneShadowFlagsTest, TerrainAlwaysCastsWhenCastNotOverridden) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TerrainComponent>(Spark::TerrainGeneratorSettings{}, Spark::Vector3::One);

    const std::int32_t flags = Spark::SceneSubmitDetail::ResolveDrawableShadowFlags(
            object,
            Spark::SceneMeshSlot::Custom,
            Spark::Matrix4::Identity,
            nullptr,
            0);

    EXPECT_EQ(flags & Spark::kSceneShadowCast, Spark::kSceneShadowCast);
}

TEST(SceneShadowFlagsTest, SubmergedGroundPlaneDoesNotCast) {
    Spark::Matrix4 world = Spark::Matrix4::Identity;
    world.m[13] = -5.0F;

    const std::int32_t flags = Spark::SceneSubmitDetail::ResolveDrawableShadowFlags(
            nullptr,
            Spark::SceneMeshSlot::GroundPlane,
            world,
            nullptr,
            kDefaultFlags);

    EXPECT_EQ(flags & Spark::kSceneShadowCast, 0);
    EXPECT_EQ(flags & Spark::kSceneShadowReceive, Spark::kSceneShadowReceive);
}

TEST(SceneShadowFlagsTest, MaterialCastOverrideWinsOverHeuristics) {
    Spark::Matrix4 world = Spark::Matrix4::Identity;
    world.m[13] = -5.0F;

    Spark::MaterialComponent material{};
    material.SetShadowCastOverride(true);

    const std::int32_t flags = Spark::SceneSubmitDetail::ResolveDrawableShadowFlags(
            nullptr,
            Spark::SceneMeshSlot::GroundPlane,
            world,
            &material,
            kDefaultFlags);

    EXPECT_EQ(flags & Spark::kSceneShadowCast, Spark::kSceneShadowCast);
}

TEST(SceneShadowFlagsTest, MaterialCanDisableReceive) {
    Spark::MaterialComponent material{};
    material.SetShadowReceiveOverride(false);

    Spark::Matrix4 world = Spark::Matrix4::Identity;
    world.m[13] = 4.0F;  // above underwater-cube heuristic (Y < 1)

    const std::int32_t flags = Spark::SceneSubmitDetail::ResolveDrawableShadowFlags(
            nullptr,
            Spark::SceneMeshSlot::UnitCube,
            world,
            &material,
            kDefaultFlags);

    EXPECT_EQ(flags & Spark::kSceneShadowReceive, 0);
    EXPECT_EQ(flags & Spark::kSceneShadowCast, Spark::kSceneShadowCast);
}
