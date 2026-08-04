#include "spark/demo/ImGuiShowcaseDemo.hpp"

#include "spark/config.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/runtime/UiSystem.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <cstdio>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark {

namespace {

void BindExposure(void* userData, const float value) {
    if (userData != nullptr) {
        *static_cast<float*>(userData) = value;
    }
}

}  // namespace

void ImGuiShowcaseDemo::Enter(IEngineContext& context) {
    DemoGui::ActivateDearImGuiDemoUi(context);
    context.GetInput().SetCursorCaptured(false);
}

void ImGuiShowcaseDemo::Leave(IEngineContext& /*context*/, GameWorld& world) noexcept {
    if (uiRoot != nullptr) {
        world.DestroyGameObject(uiRoot);
        uiRoot = nullptr;
        uiCanvas = nullptr;
        exposureSlider = nullptr;
        frameLabel = nullptr;
        uiBuilt = false;
    }
}

void ImGuiShowcaseDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    lastFrameTiming = timing;
    if (frameLabel != nullptr) {
        char frameMs[64];
        std::snprintf(frameMs, sizeof(frameMs), "Frame %.3f ms", timing.deltaTimeSeconds * 1000.0F);
        frameLabel->SetText(Utf8String(frameMs));
    }
    (void)context;
}

void ImGuiShowcaseDemo::BuildRetainedUi(GameWorld& world) {
    if (uiBuilt) {
        return;
    }
    uiRoot = world.CreateGameObject();
    uiRoot->GetName() = Utf8String("ImGuiShowcaseUi");
    uiCanvas = uiRoot->AddComponent<UiCanvasComponent>();
    uiCanvas->SetSortOrder(180);
    uiCanvas->SetTheme(Ui::UiTheme::ClassicMint());

    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::DockWorkspaceDesc dockDesc{};
    dockDesc.id = Utf8String("imgui_showcase_dock");
    dockDesc.leftWidth = 260.0F;
    dockDesc.rightWidth = 300.0F;
    auto dock = factory.CreateDockWorkspace(dockDesc);

    if (Ui::IUiElement* leftPane = dock->GetLeftPane()) {
        Ui::PanelDesc panelDesc{};
        panelDesc.id = Utf8String("hierarchy");
        panelDesc.title = Utf8String("Hierarchy");
        auto panel = factory.CreatePanel(panelDesc);
        Ui::LabelDesc sceneDesc{};
        sceneDesc.id = Utf8String("scene");
        sceneDesc.text = Utf8String("Scene");
        AdoptUiChild(*panel, factory.CreateLabel(sceneDesc));
        Ui::SeparatorDesc sepDesc{};
        sepDesc.id = Utf8String("sep");
        AdoptUiChild(*panel, factory.CreateSeparator(sepDesc));
        const char* nodes[] = {"Main Camera", "Directional Light", "Player", "Terrain"};
        for (int i = 0; i < 4; ++i) {
            Ui::LabelDesc nodeDesc{};
            nodeDesc.id = Utf8String(nodes[i]);
            nodeDesc.text = Utf8String(nodes[i]);
            nodeDesc.muted = true;
            AdoptUiChild(*panel, factory.CreateLabel(nodeDesc));
        }
        AdoptUiChild(*leftPane, MoveTemp(panel));
    }

    if (Ui::IUiElement* rightPane = dock->GetRightPane()) {
        Ui::PanelDesc panelDesc{};
        panelDesc.id = Utf8String("inspector");
        panelDesc.title = Utf8String("Inspector");
        auto panel = factory.CreatePanel(panelDesc);
        Ui::LabelDesc transformDesc{};
        transformDesc.id = Utf8String("transform_hdr");
        transformDesc.text = Utf8String("Transform");
        AdoptUiChild(*panel, factory.CreateLabel(transformDesc));
        Ui::LabelDesc posDesc{};
        posDesc.id = Utf8String("position");
        posDesc.text = Utf8String("Position: 0.0, 1.2, 4.0");
        posDesc.muted = true;
        AdoptUiChild(*panel, factory.CreateLabel(posDesc));
        Ui::SeparatorDesc sepDesc{};
        sepDesc.id = Utf8String("sep2");
        AdoptUiChild(*panel, factory.CreateSeparator(sepDesc));
        Ui::SliderDesc exposureDesc{};
        exposureDesc.id = Utf8String("exposure");
        exposureDesc.label = Utf8String("Exposure");
        exposureDesc.value = sceneExposure;
        exposureDesc.minValue = 0.1F;
        exposureDesc.maxValue = 3.0F;
        auto exposureUp = factory.CreateSlider(exposureDesc);
        exposureSlider = exposureUp.Get();
        Ui::UiFloatCallback exposureCb{};
        exposureCb.fn = &BindExposure;
        exposureCb.userData = &sceneExposure;
        exposureSlider->SetOnChanged(exposureCb);
        AdoptUiChild(*panel, MoveTemp(exposureUp));
        Ui::SeparatorDesc consoleSep{};
        consoleSep.id = Utf8String("console_sep");
        AdoptUiChild(*panel, factory.CreateSeparator(consoleSep));
        Ui::LabelDesc introDesc{};
        introDesc.id = Utf8String("intro");
        introDesc.text = Utf8String("Retained ImguiDockWorkspace + UiCanvasComponent.");
        introDesc.muted = true;
        AdoptUiChild(*panel, factory.CreateLabel(introDesc));
        Ui::LabelDesc frameDesc{};
        frameDesc.id = Utf8String("frame");
        frameDesc.text = Utf8String("Frame — ms");
        auto frameUp = factory.CreateLabel(frameDesc);
        frameLabel = frameUp.Get();
        AdoptUiChild(*panel, MoveTemp(frameUp));
        AdoptUiChild(*rightPane, MoveTemp(panel));
    }

    if (Ui::IUiElement* centerPane = dock->GetCenterPane()) {
        Ui::PanelDesc panelDesc{};
        panelDesc.id = Utf8String("viewport");
        panelDesc.title = Utf8String("Viewport");
        auto panel = factory.CreatePanel(panelDesc);
        Ui::LabelDesc hintDesc{};
        hintDesc.id = Utf8String("hint");
        hintDesc.text = Utf8String("Center dock pane (passthrough). Drag dock splits with Dear ImGui.");
        hintDesc.muted = true;
        AdoptUiChild(*panel, factory.CreateLabel(hintDesc));
        AdoptUiChild(*centerPane, MoveTemp(panel));
    }

    uiCanvas->SetRoot(MoveTemp(dock));
    uiBuilt = true;
}

void ImGuiShowcaseDemo::PaintOverlayWindows() {
#if SPARK_ENABLE_IMGUI
    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
    if (showMetrics) {
        ImGui::ShowMetricsWindow(&showMetrics);
    }
#else
    (void)showDemoWindow;
    (void)showMetrics;
#endif
}

void ImGuiShowcaseDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    BuildRetainedUi(world);

    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }

    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    SceneRenderParams params{};
    params.viewProjection = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.12F, 400.0F) * Matrix4::Identity;
    params.cameraPositionWorld = {0.0F, 2.0F, 8.0F};
    params.lightDirectionWorld = Vector3{0.3F, 0.85F, 0.4F}.Normalized();
    params.lightColor = {1.0F, 1.0F, 1.0F};
    params.lightIntensity = 0.0F;
    const Ui::UiTheme menuSkin = Ui::ResolveUiTheme(Ui::GetActiveUiThemePreset());
    params.ambientColor = {
            menuSkin.shellBackdropBottom.x * 0.14F,
            menuSkin.shellBackdropBottom.y * 0.14F,
            menuSkin.shellBackdropBottom.z * 0.14F};
    params.exposure = sceneExposure;
    params.uiFont = world.GetUiFont();
    params.uiBoldFont = world.GetUiBoldFont();
    params.uiPaintOrderNext = 0U;

    Ui::UiSystem::Get().Paint(world, params, fbW, fbH);
    PaintOverlayWindows();
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
