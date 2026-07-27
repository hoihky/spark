#include "spark/demo/ImGuiShowcaseDemo.hpp"

#include "spark/config.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"

#include <cstdio>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark {

void ImGuiShowcaseDemo::Enter(IEngineContext& context) {
    Gui::GuiToolkitSettings::SetPreferred(Gui::GuiToolkitKind::DearImGui);
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(layer->IsAvailable());
    }
#if SPARK_ENABLE_IMGUI
    if (!styleScaled) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = DemoGui::kImGuiShowcaseUiScale;
        style.ScaleAllSizes(DemoGui::kImGuiShowcaseUiScale);
        styleScaled = true;
    }
#endif
    context.GetInput().SetCursorCaptured(false);
    (void)context;
}

void ImGuiShowcaseDemo::Leave(IEngineContext& context) noexcept {
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(false);
    }
#if SPARK_ENABLE_IMGUI
    if (styleScaled) {
        const float inv = 1.0F / DemoGui::kImGuiShowcaseUiScale;
        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = 1.0F;
        style.ScaleAllSizes(inv);
        styleScaled = false;
    }
#endif
    Gui::GuiToolkitSettings::SetPreferred(Gui::GuiToolkitKind::SparkNative);
    (void)context;
}

void ImGuiShowcaseDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    lastFrameTiming = timing;
    (void)context;
}

void ImGuiShowcaseDemo::BuildToolUi(const FrameTiming& timing, IEngineContext& context) {
#if SPARK_ENABLE_IMGUI
    if (IImGuiLayer* layer = context.TryGetImGuiLayer(); layer == nullptr || !layer->IsEnabled()) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags rootFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    rootFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove;
    rootFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    ImGui::Begin("SparkImGuiRoot", nullptr, rootFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Use ESC to return to launcher", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("ImGui demo", nullptr, &showDemoWindow);
            ImGui::MenuItem("Metrics", nullptr, &showMetrics);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("SparkToolDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Hierarchy###SparkHierarchy")) {
        ImGui::TextUnformatted("Scene");
        ImGui::Separator();
        ImGui::TextUnformatted("Main Camera");
        ImGui::TextUnformatted("Directional Light");
        ImGui::TextUnformatted("Player");
        ImGui::TextUnformatted("Terrain");
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector###SparkInspector")) {
        ImGui::TextUnformatted("Transform");
        static float position[3] = {0.0F, 1.2F, 4.0F};
        ImGui::DragFloat3("Position", position, 0.05F);
        static float rotation[3] = {0.0F, 45.0F, 0.0F};
        ImGui::DragFloat3("Rotation", rotation, 0.5F);
        ImGui::Separator();
        ImGui::SliderFloat("Exposure", &sceneExposure, 0.1F, 3.0F);
        const char* tabs[] = {"Gameplay", "Rendering", "Audio"};
        ImGui::Combo("Tool tab", &selectedToolTab, tabs, 3);
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Console###SparkConsole")) {
        ImGui::TextUnformatted(
                "Dear ImGui docking showcase. Set GuiToolkitSettings::SetPreferred(SparkNative) for Spark widgets.");
        ImGui::Separator();
        char frameMs[64];
        std::snprintf(frameMs, sizeof(frameMs), "Frame %.3f ms", timing.deltaTimeSeconds * 1000.0F);
        ImGui::TextUnformatted(frameMs);
        ImGui::TextUnformatted("UI backend: Dear ImGui");
    }
    ImGui::End();

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
    if (showMetrics) {
        ImGui::ShowMetricsWindow(&showMetrics);
    }
#else
    (void)timing;
    (void)context;
#endif
}

void ImGuiShowcaseDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    BuildToolUi(lastFrameTiming, context);

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
    const Gui::GuiTheme menuSkin = Gui::ResolveGuiTheme(Gui::GetActiveGuiThemePreset());
    params.ambientColor = {
            menuSkin.shellBackdropBottom.x * 0.14F,
            menuSkin.shellBackdropBottom.y * 0.14F,
            menuSkin.shellBackdropBottom.z * 0.14F};
    params.exposure = sceneExposure;
    params.uiFont = world.GetUiFont();
    params.uiBoldFont = world.GetUiBoldFont();
    context.SetSceneRenderParams(params);
    (void)world;
}

}  // namespace Spark
