#include "spark/config.hpp"
#include "spark/gui/api/DearImGuiGuiBackend.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiInputState.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/scene/GameWorld.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#include <cstdio>
#endif

namespace Spark::Gui {

void DearImGuiGuiBackend::ProcessInput(
        GameWorld& /*world*/,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float /*contentScaleX*/,
        const float /*contentScaleY*/) {
    GuiFrameInput fin{};
    input.GetCursorFramebufferPixels(fin.mouseX, fin.mouseY, framebufferWidth, framebufferHeight);
    GuiPointerState ptrState{};
    ptrState.mouseX = fin.mouseX;
    ptrState.mouseY = fin.mouseY;
    if (imguiLayer.IsEnabled()) {
        ptrState.pointerOverGui = imguiLayer.WantsCaptureMouse();
        ptrState.consumesGamePointer = imguiLayer.WantsCaptureMouse() || imguiLayer.WantsCaptureKeyboard();
    }
    SetGuiPointerState(ptrState);
}

void DearImGuiGuiBackend::Paint(
        const GameWorld& /*world*/,
        SceneRenderParams& /*params*/,
        const int /*framebufferWidth*/,
        const int /*framebufferHeight*/) {
    // Dear ImGui records draw lists during OnEnginePostRender's prior Render() call.
}

void DearImGuiGuiBackend::BeginImmediateFrame(const GuiFrameContext& /*context*/) {
#if SPARK_ENABLE_IMGUI
    (void)immediateFrame;
#endif
}

void DearImGuiGuiBackend::EndImmediateFrame() {}

bool DearImGuiGuiBackend::WantsCaptureMouse() const noexcept {
    return imguiLayer.IsEnabled() && imguiLayer.WantsCaptureMouse();
}

bool DearImGuiGuiBackend::WantsCaptureKeyboard() const noexcept {
    return imguiLayer.IsEnabled() && imguiLayer.WantsCaptureKeyboard();
}

void DearImGuiGuiBackend::OnEnginePreRender(Window& window, IInput& input, const float deltaTimeSeconds) {
    if (!imguiLayer.IsEnabled()) {
        return;
    }
    const ImGuiFrameTiming timing{deltaTimeSeconds};
    imguiLayer.BeginFrame(window, input, timing);
}

void DearImGuiGuiBackend::OnEnginePostRender() {
    if (!imguiLayer.IsEnabled()) {
        return;
    }
    imguiLayer.EndFrame();
}

#if SPARK_ENABLE_IMGUI

void DearImGuiGuiFrame::Text(const char* text) {
    ImGui::TextUnformatted(text);
}

void DearImGuiGuiFrame::TextDisabled(const char* text) {
    ImGui::TextDisabled("%s", text);
}

void DearImGuiGuiFrame::Separator() {
    ImGui::Separator();
}

bool DearImGuiGuiFrame::Button(const char* id, const char* label) {
    ImGui::PushID(id);
    const bool pressed = ImGui::Button(label);
    ImGui::PopID();
    return pressed;
}

bool DearImGuiGuiFrame::Checkbox(const char* id, const char* label, bool& value) {
    ImGui::PushID(id);
    const bool changed = ImGui::Checkbox(label, &value);
    ImGui::PopID();
    return changed;
}

bool DearImGuiGuiFrame::SliderFloat(
        const char* id,
        const char* label,
        float& value,
        const float minValue,
        const float maxValue) {
    ImGui::PushID(id);
    const bool changed = ImGui::SliderFloat(label, &value, minValue, maxValue);
    ImGui::PopID();
    return changed;
}

bool DearImGuiGuiFrame::DragFloat3(const char* id, const char* label, float values[3], const float speed) {
    ImGui::PushID(id);
    const bool changed = ImGui::DragFloat3(label, values, speed);
    ImGui::PopID();
    return changed;
}

bool DearImGuiGuiFrame::Combo(
        const char* id,
        const char* label,
        int& currentItem,
        const char* const items[],
        const int itemCount) {
    ImGui::PushID(id);
    const bool changed = ImGui::Combo(label, &currentItem, items, itemCount);
    ImGui::PopID();
    return changed;
}

bool DearImGuiGuiFrame::BeginPanel(const char* id, const char* title, bool* open) {
    if (open != nullptr && !*open) {
        return false;
    }
    char windowName[160];
    if (title != nullptr && id != nullptr) {
        snprintf(windowName, sizeof(windowName), "%s###%s", title, id);
    } else if (title != nullptr) {
        snprintf(windowName, sizeof(windowName), "%s", title);
    } else if (id != nullptr) {
        snprintf(windowName, sizeof(windowName), "%s", id);
    } else {
        snprintf(windowName, sizeof(windowName), "Panel");
    }
    const bool visible = ImGui::Begin(windowName, open);
    ++imguiPanelStack;
    return visible;
}

void DearImGuiGuiFrame::EndPanel() {
    if (imguiPanelStack > 0) {
        ImGui::End();
        --imguiPanelStack;
    }
}

void DearImGuiGuiFrame::SameLine(const float offsetFromStartX, const float spacing) {
    ImGui::SameLine(offsetFromStartX, spacing);
}

bool DearImGuiGuiFrame::Selectable(const char* id, const char* label, const bool selected) {
    ImGui::PushID(id);
    const bool clicked = ImGui::Selectable(label != nullptr ? label : id, selected);
    ImGui::PopID();
    return clicked;
}

bool DearImGuiGuiFrame::MenuItem(const char* label, bool* checked) {
    if (checked != nullptr) {
        return ImGui::MenuItem(label, nullptr, checked);
    }
    return ImGui::MenuItem(label);
}

void DearImGuiGuiFrame::SetCursorPos(const float x, const float y) {
    ImGui::SetCursorPos(ImVec2(x, y));
}

void DearImGuiGuiFrame::SetNextPanelSize(const float width, const float height) {
    if (width > 0.0F && height > 0.0F) {
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    } else if (width > 0.0F) {
        ImGui::SetNextWindowSize(ImVec2(width, 0.0F), ImGuiCond_Always);
    } else {
        (void)height;
    }
}

#else

void DearImGuiGuiFrame::Text(const char*) {}
void DearImGuiGuiFrame::TextDisabled(const char*) {}
void DearImGuiGuiFrame::Separator() {}
bool DearImGuiGuiFrame::Button(const char*, const char*) { return false; }
bool DearImGuiGuiFrame::Checkbox(const char*, const char*, bool&) { return false; }
bool DearImGuiGuiFrame::SliderFloat(const char*, const char*, float&, float, float) { return false; }
bool DearImGuiGuiFrame::DragFloat3(const char*, const char*, float[3], float) { return false; }
bool DearImGuiGuiFrame::Combo(const char*, const char*, int&, const char* const[], int) { return false; }
bool DearImGuiGuiFrame::BeginPanel(const char*, const char*, bool*) { return false; }
void DearImGuiGuiFrame::EndPanel() {}
void DearImGuiGuiFrame::SameLine(float, float) {}
bool DearImGuiGuiFrame::Selectable(const char*, const char*, bool) { return false; }
bool DearImGuiGuiFrame::MenuItem(const char*, bool*) { return false; }
void DearImGuiGuiFrame::SetCursorPos(float, float) {}
void DearImGuiGuiFrame::SetNextPanelSize(float, float) {}

#endif

}  // namespace Spark::Gui
