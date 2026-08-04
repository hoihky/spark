#include "spark/ui/runtime/UiScene.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/text/Font.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/core/UiPointerState.hpp"
#include "spark/ui/core/SparkUiRenderer.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"
#include "spark/ui/runtime/UiSystem.hpp"
#include "spark/ui/runtime/UiToolkitSettings.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark {

namespace {

void CollectUiCanvases(const GameWorld& world, Array<UiCanvasComponent*>& out) {
    out.Clear();
    world.ForEachActiveGameObject([&out](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (UiCanvasComponent* canvas = object->GetComponent<UiCanvasComponent>()) {
            if (canvas->IsCanvasEnabled()) {
                out.PushBack(canvas);
            }
        }
    });
}

void SortCanvasesByOrder(Array<UiCanvasComponent*>& list) {
    for (std::size_t i = 1; i < list.GetSize(); ++i) {
        UiCanvasComponent* key = list[i];
        std::size_t j = i;
        while (j > 0 && list[j - 1U]->GetSortOrder() > key->GetSortOrder()) {
            list[j] = list[j - 1U];
            --j;
        }
        list[j] = key;
    }
}

void PrepareUiCanvasFrame() {
    Ui::UiLayoutMetrics layoutMetrics = Ui::UiLayoutMetrics::Default();
    if (Ui::UiToolkitSettings::ShouldProcessSparkUiInput()) {
        layoutMetrics.uiScale = DemoGui::kImmediateUiScaleBoost;
    } else {
        layoutMetrics.uiScale = DemoGui::kDearImGuiDemoUiScale;
    }
    Ui::SetActiveUiLayoutMetrics(layoutMetrics);
}

}  // namespace

void ProcessUiCanvasesInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    (void)contentScaleX;
    (void)contentScaleY;
    PrepareUiCanvasFrame();
    Ui::UiSystem::Get().ProcessInput(
            world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void ProcessUiCanvasesInput(
        Scene& scene,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    ProcessUiCanvasesInput(
            scene.GetWorld(), input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void PaintUiCanvases(
        const GameWorld& world,
        Ui::IUiRenderer& renderer,
        const int framebufferWidth,
        const int framebufferHeight) {
    Array<UiCanvasComponent*> list;
    CollectUiCanvases(world, list);
    SortCanvasesByOrder(list);

    const float fbw = static_cast<float>(framebufferWidth > 0 ? framebufferWidth : 1);
    const float fbh = static_cast<float>(framebufferHeight > 0 ? framebufferHeight : 1);
    Ui::Rect viewport{0.0F, 0.0F, fbw, fbh};
#if SPARK_ENABLE_IMGUI
    if (!Ui::UiToolkitSettings::ShouldProcessSparkUiInput()) {
        if (const ImGuiViewport* imguiViewport = ImGui::GetMainViewport()) {
            viewport.width = imguiViewport->WorkSize.x > 0.0F ? imguiViewport->WorkSize.x : fbw;
            viewport.height = imguiViewport->WorkSize.y > 0.0F ? imguiViewport->WorkSize.y : fbh;
        }
    }
#endif

    PrepareUiCanvasFrame();

    renderer.SetLayoutMetrics(&Ui::GetActiveUiLayoutMetrics());
    if (const SharedPtr<Font>& uiFont = world.GetUiFont(); uiFont) {
        renderer.SetLayoutFont(uiFont.Get());
    }
    if (auto* sparkRenderer = dynamic_cast<Ui::SparkUiRenderer*>(&renderer)) {
        sparkRenderer->GetPaintContext().SetFramebufferPixelSize(fbw, fbh);
    }

    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        UiCanvasComponent* canvas = list[i];
        if (canvas == nullptr || !canvas->IsCanvasEnabled()) {
            continue;
        }
        Ui::IUiElement* root = canvas->GetRoot();
        if (root == nullptr || !root->IsVisible()) {
            continue;
        }
        Ui::UiMeasureConstraints constraints{};
        constraints.maxWidth = viewport.width;
        constraints.maxHeight = viewport.height;
        Ui::UiSize desired{};
        root->Measure(constraints, desired);
        (void)desired;
        root->Arrange(viewport);
        canvas->Paint(renderer);
    }

    if (Ui::GetUiContextMenu().IsOpen()) {
        if (auto* sparkRenderer = dynamic_cast<Ui::SparkUiRenderer*>(&renderer)) {
            Ui::GetUiContextMenu().Paint(sparkRenderer->GetPaintContext());
        }
    }

    (void)framebufferHeight;
}

void PaintUiCanvases(
        const GameWorld& world,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    Ui::UiSystem::Get().Paint(world, params, framebufferWidth, framebufferHeight);
}

void PaintUiCanvases(
        const Scene& scene,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    PaintUiCanvases(scene.GetWorld(), params, framebufferWidth, framebufferHeight);
}

void PaintUiCanvases(
        const Scene& scene,
        Ui::IUiRenderer& renderer,
        const int framebufferWidth,
        const int framebufferHeight) {
    PaintUiCanvases(scene.GetWorld(), renderer, framebufferWidth, framebufferHeight);
}

bool UiScrollWheelConsumed() noexcept {
    return Ui::GetUiPointerState().scrollWheelConsumed;
}

}  // namespace Spark
