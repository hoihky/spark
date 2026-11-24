#include <gtest/gtest.h>

#include "spark/core/SharedPtr.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/RenderLayerComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/world/WorldRendering.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/submit/RenderLayerRegistry.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"
#include "spark/scene/submit/SceneRenderQueue.hpp"

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

TEST(DrawableSortSceneDrawTest, ApplyToSceneDrawItemMapsOverlayLayer) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    const Spark::RenderLayerId overlayId = Spark::RenderLayerRegistry::Instance().FindLayerIdByName("Overlay");
    ASSERT_NE(overlayId, Spark::kInvalidRenderLayerId);
    object->AddComponent<Spark::RenderLayerComponent>(overlayId, 7);

    Spark::SceneDrawItem item{};
    Spark::DrawableSortResolver::ApplyToSceneDrawItem(*object, item, 0);

    EXPECT_EQ(item.renderQueue, Spark::SceneRenderQueue::Overlay);
    EXPECT_EQ(item.drawableSort.sortingLayerOrder, 400);
    EXPECT_EQ(item.drawableSort.sortingOrder, 7);
}

TEST(DrawableSortSceneDrawTest, StableSortOpaqueOrdersBackgroundBeforeOverlayMeshes) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem overlay{};
    overlay.mesh = Spark::SceneMeshSlot::UnitCube;
    overlay.renderQueue = Spark::SceneRenderQueue::Overlay;
    overlay.drawableSort.sortingLayerOrder = 400;
    Spark::SceneDrawItem background{};
    background.mesh = Spark::SceneMeshSlot::UnitCube;
    background.renderQueue = Spark::SceneRenderQueue::Background;
    background.drawableSort.sortingLayerOrder = 0;

    items.PushBack(overlay);
    items.PushBack(background);

    Spark::SceneDrawListSorter::StableSortOpaqueDraws(items);

    ASSERT_EQ(items.GetSize(), 2U);
    EXPECT_EQ(items[0].renderQueue, Spark::SceneRenderQueue::Background);
    EXPECT_EQ(items[1].renderQueue, Spark::SceneRenderQueue::Overlay);
}

TEST(DrawableSortSceneDrawTest, FillLitPropagatesRenderLayerOntoMeshDraw) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});
    const Spark::RenderLayerId overlayId = Spark::RenderLayerRegistry::Instance().FindLayerIdByName("Overlay");
    object->AddComponent<Spark::RenderLayerComponent>(overlayId, 3);
    object->AddComponent<Spark::MeshComponent>(
            Spark::SharedPtr<Spark::Mesh>{},
            Spark::SceneMeshSlot::UnitCube,
            Spark::Vector3{1.0F, 1.0F, 1.0F});

    NoOpEngineContext context{};
    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(1.0471975F, 16.0F / 9.0F, 0.1F, 500.0F);
    const Spark::Matrix4 view = Spark::Matrix4::LookAt({0.0F, 2.0F, 8.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    const Spark::Matrix4 viewProjection = proj * view;

    Spark::SceneRenderParams params{};
    Spark::LitSceneSubmitOptions options{};
    world.Rendering().FillLit(context, viewProjection, {0.0F, 2.0F, 8.0F}, options, params);

    ASSERT_EQ(params.draws.GetSize(), 1U);
    EXPECT_EQ(params.draws[0].renderQueue, Spark::SceneRenderQueue::Overlay);
    EXPECT_EQ(params.draws[0].drawableSort.sortingOrder, 3);
}

TEST(DrawableSortSceneDrawTest, PartitionMarksTransparentQueue) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem transparent{};
    transparent.opacity = 0.4F;

    items.PushBack(transparent);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, Spark::Vector3::Zero);

    ASSERT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws[0].renderQueue, Spark::SceneRenderQueue::Transparent);
}