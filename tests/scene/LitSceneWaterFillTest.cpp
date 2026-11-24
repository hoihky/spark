#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"
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

}  // namespace

TEST(LitSceneWaterFillTest, FillLitRoutesWaterBodiesToWaterDrawsOnly) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    ASSERT_NE(object, nullptr);
    Spark::WaterBodyComponent* water = object->AddComponent<Spark::WaterBodyComponent>();
    water->OnAttach(*object);
    object->AddComponent<Spark::MeshComponent>(
            Spark::SharedPtr<Spark::Mesh>{},
            Spark::SceneMeshSlot::GroundPlane,
            Spark::Vector3{24.0F, 1.0F, 24.0F});

    Spark::SceneRenderParams params{};
    NoOpEngineContext context{};
    Spark::LitSceneSubmitOptions options{};
    world.Rendering().FillLit(
            context,
            Spark::Matrix4::Identity,
            {0.0F, 0.0F, 10.0F},
            options,
            params);

    EXPECT_EQ(params.waterDraws.GetSize(), 1U);
    EXPECT_EQ(params.draws.GetSize(), 0U);
    EXPECT_EQ(params.transparentDraws.GetSize(), 0U);
}