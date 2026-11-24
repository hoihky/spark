#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/animation/SkinnedAnimationBudgetComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/physics/3d/CollisionFilter3D.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/world/WorldRendering.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <cmath>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

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

struct GoldenCamera {
    Spark::Matrix4 viewProjection{};
    Spark::Vector3 position{0.0F, 2.0F, 8.0F};
};

[[nodiscard]] GoldenCamera MakeGoldenCamera() {
    GoldenCamera cam{};
    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(1.0471975F, 16.0F / 9.0F, 0.1F, 500.0F);
    const Spark::Matrix4 view = Spark::Matrix4::LookAt(cam.position, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    cam.viewProjection = proj * view;
    return cam;
}

Spark::GameObject* AddStaticFloor(Spark::GameWorld& world) {
    Spark::GameObject* floor = world.CreateGameObject();
    floor->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, -0.5F, 0.0F});
    auto* box = floor->AddComponent<Spark::BoxCollider3DComponent>(Spark::Vector3{12.0F, 0.5F, 12.0F});
    box->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));
    box->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());
    return floor;
}

Spark::GameObject* AddPhysicsBall(Spark::GameWorld& world) {
    Spark::GameObject* ball = world.CreateGameObject();
    ball->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 5.0F, 0.0F});
    auto* sphere = ball->AddComponent<Spark::SphereCollider3DComponent>(0.25F);
    sphere->SetCategoryBits(Spark::CollisionFilter3D::DefaultCategory());
    sphere->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());
    ball->AddComponent<Spark::Rigidbody3DComponent>();
    return ball;
}

void AddOpaqueCube(Spark::GameWorld& world, const Spark::Vector3& position) {
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>()->SetTranslation(position);
    object->AddComponent<Spark::MeshComponent>(
            Spark::SharedPtr<Spark::Mesh>{},
            Spark::SceneMeshSlot::UnitCube,
            Spark::Vector3{0.5F, 0.5F, 0.5F});
}

void AddWater(Spark::GameWorld& world) {
    Spark::GameObject* object = world.CreateGameObject();
    auto* water = object->AddComponent<Spark::WaterBodyComponent>();
    water->OnAttach(*object);
    object->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, -0.2F, 0.0F});
    object->AddComponent<Spark::MeshComponent>(
            Spark::SharedPtr<Spark::Mesh>{},
            Spark::SceneMeshSlot::GroundPlane,
            Spark::Vector3{16.0F, 1.0F, 16.0F});
}

void AddSprite(Spark::GameWorld& world) {
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>()->SetTranslation({-1.5F, 0.5F, 0.0F});
    auto tex = Spark::MakeShared<Spark::Texture2D>();
    *tex = Spark::Texture2D::CreateCheckerboard(
            4, 4, Spark::Vector3{0.2F, 0.4F, 1.0F}, Spark::Vector3{0.8F, 0.9F, 1.0F});
    object->AddComponent<Spark::SpriteComponent>(tex, Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Spark::Vector4{0, 0, 1, 1}, 0);
}

void AddSkinnedFoxPair(Spark::GameWorld& world, const Spark::SkinnedGltfAsset& asset) {
    for (std::size_t i = 0; i < 2; ++i) {
        Spark::GameObject* character = world.CreateGameObject();
        character->AddComponent<Spark::TransformComponent>()->SetTranslation({static_cast<float>(i) * 2.5F, 0.0F, 0.0F});
        character->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
        character->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    }
}

NoOpEngineContext gContext{};

}  // namespace

TEST(FullFrameIntegrationGoldenTest, FixedPhysicsStepBudgetCapsAndSubmitCounts) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset foxAsset = world.Assets().LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(foxAsset.skeleton));
    ASSERT_TRUE(static_cast<bool>(foxAsset.mesh));

    Spark::GameObject* policy = world.CreateGameObject();
    auto* budgetPolicy = policy->AddComponent<Spark::SkinnedAnimationBudgetComponent>();
    budgetPolicy->SetMaxPaletteUpdatesPerFrame(1);
    budgetPolicy->SetMaxSkinnedDrawsPerFrame(2);
    budgetPolicy->SetPaletteCacheEnabled(false);

    AddStaticFloor(world);
    Spark::GameObject* ball = AddPhysicsBall(world);
    AddOpaqueCube(world, {4.0F, 0.5F, -2.0F});
    AddWater(world);
    AddSprite(world);
    AddSkinnedFoxPair(world, foxAsset);

    Spark::PhysicsSubsystem physics{};
    physics.GetWorld3D().GetSettings().gravityY = -10.0F;
    physics.GetWorld3D().GetSettings().fixedSimulationTimestep = 1.0F / 60.0F;
    physics.GetWorld3D().GetSettings().maxFixedSimulationStepsPerFrame = 4;
    physics.GetWorld3D().GetSettings().substeps = 1;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;
    physics.SimulateAll3D(world, timing);

    const float ballY = ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.y;
    EXPECT_NEAR(ballY, 4.99722242F, 1.0e-4F);

    const Spark::SkinnedAnimationBudget& budgetBeforeSubmit = world.GetSkinnedAnimationService().GetBudget();
    EXPECT_EQ(budgetBeforeSubmit.GetPaletteUpdatesUsed(), 0U);
    EXPECT_EQ(budgetBeforeSubmit.GetSkinnedDrawsSubmitted(), 0U);

    Spark::SceneRenderParams params{};
    const GoldenCamera camera = MakeGoldenCamera();
    Spark::LitSceneSubmitOptions options{};
    world.Rendering().FillLit(gContext, camera.viewProjection, camera.position, options, params);

    EXPECT_EQ(params.draws.GetSize(), 3U);
    EXPECT_EQ(params.waterDraws.GetSize(), 1U);
    EXPECT_EQ(params.sprites.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws.GetSize(), 0U);

    const Spark::SkinnedAnimationBudget& budget = world.GetSkinnedAnimationService().GetBudget();
    EXPECT_EQ(budget.GetPaletteUpdatesUsed(), 1U);
    EXPECT_EQ(budget.GetPaletteFallbacksToBindPose(), 1U);
    EXPECT_EQ(budget.GetSkinnedDrawsSubmitted(), 2U);
}