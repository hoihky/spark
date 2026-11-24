#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/core/ScenePartitionKind.hpp"
#include "spark/scene/core/SceneDrawableFrustumSink.hpp"
#include "spark/scene/core/world/WorldRendering.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"

namespace {

class NoOpEngineContext final : public Spark::IEngineContext {
public:
    Spark::Window& GetWindow() override { std::terminate(); }
    Spark::IFramePresenter& GetFramePresenter() override { std::terminate(); }
    Spark::IInput& GetInput() override { std::terminate(); }
    void GetFramebufferSize(int& outWidth, int& outHeight) const override {
        outWidth = 1280;
        outHeight = 720;
    }
    void SetSceneRenderParams(const Spark::SceneRenderParams& /*params*/) override {}
    Spark::SoundEngine* TryGetSoundEngine() noexcept override { return nullptr; }
    Spark::Scene* TryGetScene() noexcept override { return nullptr; }
    Spark::IImGuiLayer* TryGetImGuiLayer() noexcept override { return nullptr; }
};

void AddUnitCube(Spark::GameWorld& world, const Spark::Vector3& position) {
    Spark::GameObject* object = world.CreateGameObject();
    if (object == nullptr) {
        return;
    }
    Spark::TransformComponent* transform = object->GetComponent<Spark::TransformComponent>();
    if (transform == nullptr) {
        transform = object->AddComponent<Spark::TransformComponent>();
    }
    transform->SetTranslation(position);
    object->AddComponent<Spark::MeshComponent>(
            Spark::SharedPtr<Spark::Mesh>{},
            Spark::SceneMeshSlot::UnitCube,
            Spark::Vector3{0.6F, 0.6F, 0.6F});
}

[[nodiscard]] std::size_t CountMeshDraws(const Spark::SceneRenderParams& params) {
    return params.draws.GetSize() + params.transparentDraws.GetSize();
}

struct LitSceneFrustumSetup {
    Spark::Matrix4 viewProjection{};
    Spark::Vector3 cameraPosition{};
};

[[nodiscard]] LitSceneFrustumSetup MakeLitSceneFrustumSetup() {
    LitSceneFrustumSetup out{};
    out.cameraPosition = {0.0F, 2.0F, 8.0F};
    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(1.0471975F, 16.0F / 9.0F, 0.1F, 500.0F);
    const Spark::Matrix4 view = Spark::Matrix4::LookAt(out.cameraPosition, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    out.viewProjection = proj * view;
    return out;
}

void FillLitScene(
        Spark::GameWorld& world,
        Spark::IEngineContext& context,
        const LitSceneFrustumSetup& frustum,
        const Spark::LitSceneSubmitOptions& options,
        Spark::SceneRenderParams& outParams) {
    world.Rendering().FillLit(
            context, frustum.viewProjection, frustum.cameraPosition, options, outParams);
}

}  // namespace

struct CountingDrawableSink final : Spark::DrawableFrustumSink {
    int count = 0;
    void OnDrawable(
            Spark::GameObject* /*object*/,
            const Spark::MeshComponent& /*mesh*/,
            const Spark::MaterialComponent* /*material*/,
            const Spark::Matrix4& /*worldMatrix*/) override {
        ++count;
    }
};

TEST(LitSceneFrustumCullTest, DispatchBruteForceDropsOutsideFrustum) {
    const LitSceneFrustumSetup frustum = MakeLitSceneFrustumSetup();
    Spark::Scene scene;
    scene.SetSpatialPartitionKind(Spark::ScenePartitionKind::BruteForce);
    Spark::GameWorld& world = scene.GetWorld();

    AddUnitCube(world, {0.0F, 0.0F, 0.0F});
    AddUnitCube(world, {120.0F, 0.0F, 0.0F});

    CountingDrawableSink sink;
    Spark::DispatchDrawableFrustumCull(
            scene, frustum.viewProjection, Spark::ScenePartitionKind::BruteForce, sink);
    EXPECT_EQ(sink.count, 1);
}

class LitSceneFrustumCullTest : public ::testing::TestWithParam<Spark::ScenePartitionKind> {};

TEST_P(LitSceneFrustumCullTest, FrustumCullOmitsDrawablesBehindCamera) {
    const Spark::ScenePartitionKind partition = GetParam();
    ASSERT_NE(partition, Spark::ScenePartitionKind::None);

    Spark::Scene scene;
    scene.SetSpatialPartitionKind(partition);
    Spark::GameWorld& world = scene.GetWorld();

    const LitSceneFrustumSetup frustum = MakeLitSceneFrustumSetup();
    const Spark::Frustum viewFrustum = Spark::Frustum::FromColumnMajorViewProjection(frustum.viewProjection);
    const Spark::AxisAlignedBox visible = Spark::AxisAlignedBox::FromMinMax(
            {-0.5F, -0.5F, -0.5F},
            {0.5F, 0.5F, 0.5F});
    Spark::Matrix4 farWorld = Spark::Matrix4::Identity;
    farWorld.m[12] = 120.0F;
    Spark::Vector3 farMin{};
    Spark::Vector3 farMax{};
    Spark::AxisAlignedBox::EncapsulateTransformedLocal(
            {-0.5F, -0.5F, -0.5F},
            {0.5F, 0.5F, 0.5F},
            farWorld,
            farMin,
            farMax);
    const Spark::AxisAlignedBox culledOut = Spark::AxisAlignedBox::FromMinMax(farMin, farMax);
    ASSERT_TRUE(viewFrustum.Intersects(visible));
    ASSERT_FALSE(viewFrustum.Intersects(culledOut));

    AddUnitCube(world, {0.0F, 0.0F, 0.0F});
    AddUnitCube(world, {120.0F, 0.0F, 0.0F});

    NoOpEngineContext context;

    Spark::SceneRenderParams unculled{};
    Spark::LitSceneSubmitOptions noScene{};
    noScene.WithSceneForCulling(nullptr);
    FillLitScene(world, context, frustum, noScene, unculled);

    Spark::SceneRenderParams culled{};
    Spark::LitSceneSubmitOptions withScene{};
    withScene.WithSceneForCulling(&scene);
    FillLitScene(world, context, frustum, withScene, culled);

    EXPECT_EQ(CountMeshDraws(unculled), 2U);
    EXPECT_EQ(CountMeshDraws(culled), 1U);
}

INSTANTIATE_TEST_SUITE_P(
        PartitionModes,
        LitSceneFrustumCullTest,
        ::testing::Values(
                Spark::ScenePartitionKind::BoundingVolumeHierarchy,
                Spark::ScenePartitionKind::Octree,
                Spark::ScenePartitionKind::QuadTree));

TEST(LitSceneFrustumCullTest, ScenePartitionNoneMatchesUnculledWalk) {
    Spark::Scene scene;
    scene.SetSpatialPartitionKind(Spark::ScenePartitionKind::None);
    Spark::GameWorld& world = scene.GetWorld();

    const LitSceneFrustumSetup frustum = MakeLitSceneFrustumSetup();
    const Spark::Frustum viewFrustum = Spark::Frustum::FromColumnMajorViewProjection(frustum.viewProjection);
    const Spark::AxisAlignedBox visible = Spark::AxisAlignedBox::FromMinMax(
            {-0.5F, -0.5F, -0.5F},
            {0.5F, 0.5F, 0.5F});
    Spark::Matrix4 farWorld = Spark::Matrix4::Identity;
    farWorld.m[12] = 120.0F;
    Spark::Vector3 farMin{};
    Spark::Vector3 farMax{};
    Spark::AxisAlignedBox::EncapsulateTransformedLocal(
            {-0.5F, -0.5F, -0.5F},
            {0.5F, 0.5F, 0.5F},
            farWorld,
            farMin,
            farMax);
    const Spark::AxisAlignedBox culledOut = Spark::AxisAlignedBox::FromMinMax(farMin, farMax);
    ASSERT_TRUE(viewFrustum.Intersects(visible));
    ASSERT_FALSE(viewFrustum.Intersects(culledOut));

    AddUnitCube(world, {0.0F, 0.0F, 0.0F});
    AddUnitCube(world, {120.0F, 0.0F, 0.0F});

    NoOpEngineContext context;

    Spark::SceneRenderParams unculled{};
    Spark::LitSceneSubmitOptions noScene{};
    noScene.WithSceneForCulling(nullptr);
    FillLitScene(world, context, frustum, noScene, unculled);

    Spark::SceneRenderParams withScene{};
    Spark::LitSceneSubmitOptions sceneOpt{};
    sceneOpt.WithSceneForCulling(&scene);
    FillLitScene(world, context, frustum, sceneOpt, withScene);

    EXPECT_EQ(CountMeshDraws(unculled), CountMeshDraws(withScene));
    EXPECT_EQ(CountMeshDraws(withScene), 2U);
}