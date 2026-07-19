#include "spark/demo/ImGuiShowcaseDemo.hpp"

#include "spark/config.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/GuiScene.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark {

void ImGuiShowcaseDemo::Enter(IEngineContext& context) {
    Gui::GuiToolkitSettings::SetPreferred(Gui::GuiToolkitKind::DearImGui);
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(layer->IsAvailable());
    }
    context.GetInput().SetCursorCaptured(false);
    (void)context;
}

void ImGuiShowcaseDemo::Leave(IEngineContext& context) noexcept {
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(false);
    }
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
    ImGui::SetNextWindowViewport(viewport->ID);
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
            ImGui::TextDisabled("Use ESC to return to launcher");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("ImGui demo", nullptr, &showDemoWindow);
            ImGui::MenuItem("Metrics", nullptr, &showMetrics);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiID dockspaceId = ImGui::GetID("SparkToolDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::Begin("Hierarchy")) {
        ImGui::TextUnformatted("Scene");
        ImGui::Separator();
        ImGui::Selectable("Main Camera", true);
        ImGui::Selectable("Directional Light");
        ImGui::Selectable("Player");
        ImGui::Selectable("Terrain");
    }
    ImGui::End();

    if (ImGui::Begin("Inspector")) {
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

    if (ImGui::Begin("Console")) {
        ImGui::TextWrapped(
                "Dear ImGui docking branch is active. Spark retained GUI remains available when "
                "GuiToolkitSettings::SetPreferred(SparkNative) and ImGuiLayer::SetEnabled(false).");
        ImGui::Separator();
        ImGui::Text("Frame %.3f ms", timing.deltaTimeSeconds * 1000.0F);
        ImGui::Text("UI toolkit: Dear ImGui");
    }
    ImGui::End();

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
