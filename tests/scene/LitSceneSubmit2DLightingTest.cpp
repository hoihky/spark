#include <gtest/gtest.h>

#include "spark/core/SharedPtr.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/lighting/PointLight2DComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/world/WorldRendering.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/texture/Texture2D.hpp"

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

struct Lit2DCamera {
    Spark::Matrix4 viewProjection{};
    Spark::Vector3 position{0.0F, 2.0F, 8.0F};
};

[[nodiscard]] Lit2DCamera MakeLit2DCamera() {
    Lit2DCamera cam{};
    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(1.0471975F, 16.0F / 9.0F, 0.1F, 500.0F);
    const Spark::Matrix4 view = Spark::Matrix4::LookAt(cam.position, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    cam.viewProjection = proj * view;
    return cam;
}

void FillLit2D(Spark::GameWorld& world, Spark::SceneRenderParams& params) {
    NoOpEngineContext context{};
    const Lit2DCamera cam = MakeLit2DCamera();
    Spark::LitSceneSubmitOptions options{};
    world.Rendering().FillLit(context, cam.viewProjection, cam.position, options, params);
}

[[nodiscard]] Spark::SharedPtr<Spark::Texture2D> MakeCheckerTexture() {
    auto tex = Spark::MakeShared<Spark::Texture2D>();
    *tex = Spark::Texture2D::CreateCheckerboard(
            4, 4, Spark::Vector3{1.0F, 0.0F, 0.0F}, Spark::Vector3{0.0F, 0.0F, 1.0F});
    return tex;
}

NoOpEngineContext gEngine{};

}  // namespace

TEST(LitSceneSubmit2DLightingTest, FillLitCollectsPointLight2DIntoPunctualLights) {
    Spark::GameWorld world{};
    Spark::GameObject* lamp = world.CreateGameObject();
    lamp->AddComponent<Spark::TransformComponent>()->SetTranslation({3.0F, 4.0F, 0.0F});
    auto* light = lamp->AddComponent<Spark::PointLight2DComponent>(Spark::Vector3{0.9F, 0.7F, 0.5F}, 3.25F, 8.0F);
    light->SetLocalOffset({0.5F, -0.25F});
    light->SetWorldZ(0.5F);

    Spark::SceneRenderParams params{};
    FillLit2D(world, params);

    ASSERT_EQ(params.pointLights.GetSize(), 1U);
    const Spark::ScenePointLight& gpu = params.pointLights[0];
    EXPECT_FLOAT_EQ(gpu.positionWorld.x, 3.5F);
    EXPECT_FLOAT_EQ(gpu.positionWorld.y, 3.75F);
    EXPECT_FLOAT_EQ(gpu.positionWorld.z, 0.5F);
    EXPECT_FLOAT_EQ(gpu.range, 8.0F);
    EXPECT_FLOAT_EQ(gpu.intensity, 3.25F);
    EXPECT_FLOAT_EQ(gpu.color.x, 0.9F);
    EXPECT_FLOAT_EQ(gpu.color.y, 0.7F);
    EXPECT_FLOAT_EQ(gpu.color.z, 0.5F);
    EXPECT_FALSE(gpu.castsShadow);
}

TEST(LitSceneSubmit2DLightingTest, FillLitSkipsDisabledPointLight2D) {
    Spark::GameWorld world{};
    Spark::GameObject* lamp = world.CreateGameObject();
    auto* light = lamp->AddComponent<Spark::PointLight2DComponent>();
    light->SetEnabled(false);

    Spark::SceneRenderParams params{};
    FillLit2D(world, params);
    EXPECT_TRUE(params.pointLights.IsEmpty());
}

TEST(LitSceneSubmit2DLightingTest, FillLitPointLight2DUsesEffectiveIntensityAfterFlickerUpdate) {
    Spark::GameWorld world{};
    Spark::GameObject* lamp = world.CreateGameObject();
    auto* light = lamp->AddComponent<Spark::PointLight2DComponent>(Spark::Vector3::One, 4.0F, 6.0F);
    light->SetFlicker(true, 0.5F, 60.0F);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 120.0F;
    world.UpdateGameObjects(timing, gEngine);
    EXPECT_LT(light->GetEffectiveIntensity(), 4.0F);

    Spark::SceneRenderParams params{};
    FillLit2D(world, params);
    ASSERT_EQ(params.pointLights.GetSize(), 1U);
    EXPECT_FLOAT_EQ(params.pointLights[0].intensity, light->GetEffectiveIntensity());
}

TEST(LitSceneSubmit2DLightingTest, FillLitSpriteLightingCopiesModeAndParams) {
    Spark::GameWorld world{};
    Spark::GameObject* spriteObject = world.CreateGameObject();
    spriteObject->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});
    const Spark::SharedPtr<Spark::Texture2D> albedo = MakeCheckerTexture();
    spriteObject->AddComponent<Spark::SpriteComponent>(
            albedo,
            Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            0);

    auto* lit = spriteObject->AddComponent<Spark::SpriteLighting2DComponent>(
            Spark::SpriteLighting2DMode::PointSoft,
            Spark::Vector4{0.2F, 0.85F, 0.0F, 0.0F},
            Spark::Vector4{1.0F, 2.0F, 3.0F, 4.0F});

    Spark::SceneRenderParams params{};
    FillLit2D(world, params);

    ASSERT_EQ(params.sprites.GetSize(), 1U);
    const Spark::SceneSpriteDraw& draw = params.sprites[0];
    EXPECT_EQ(draw.lightingMode, Spark::SpriteLighting2DMode::PointSoft);
    EXPECT_FLOAT_EQ(draw.lightingParam0.x, 0.2F);
    EXPECT_FLOAT_EQ(draw.lightingParam0.y, 0.85F);
    EXPECT_FLOAT_EQ(draw.lightingParam1.x, 1.0F);
    EXPECT_FLOAT_EQ(draw.lightingParam1.y, 2.0F);
    EXPECT_EQ(lit->GetMode(), Spark::SpriteLighting2DMode::PointSoft);
}

TEST(LitSceneSubmit2DLightingTest, FillLitSpriteLightingMapsNormalAndRampLayers) {
    Spark::GameWorld world{};
    Spark::GameObject* spriteObject = world.CreateGameObject();
    spriteObject->AddComponent<Spark::TransformComponent>()->SetTranslation({1.0F, 0.0F, 0.0F});
    const Spark::SharedPtr<Spark::Texture2D> albedo = MakeCheckerTexture();
    spriteObject->AddComponent<Spark::SpriteComponent>(
            albedo, Spark::Vector4::One, Spark::Vector4{0.1F, 0.2F, 0.9F, 1.0F}, 0);

    auto* lit = spriteObject->AddComponent<Spark::SpriteLighting2DComponent>(
            Spark::SpriteLighting2DMode::Ramp);
    lit->SetNormalMap(MakeCheckerTexture());
    lit->SetRampMap(MakeCheckerTexture());
    lit->SetSyncNormalUvWithSprite(false);
    lit->SetNormalUvRect({0.25F, 0.25F, 0.75F, 0.75F});

    Spark::SceneRenderParams params{};
    FillLit2D(world, params);

    ASSERT_EQ(params.sprites.GetSize(), 1U);
    const Spark::SceneSpriteDraw& draw = params.sprites[0];
    EXPECT_GE(draw.textureLayer, 0);
    EXPECT_GE(draw.normalTextureLayer, 0);
    EXPECT_GE(draw.rampTextureLayer, 0);
    EXPECT_FLOAT_EQ(draw.normalUvRect.x, 0.25F);
    EXPECT_FLOAT_EQ(draw.normalUvRect.y, 0.25F);
    EXPECT_FLOAT_EQ(draw.uvRect.z, 0.9F);
    EXPECT_FLOAT_EQ(draw.normalUvRect.z, 0.75F);
}