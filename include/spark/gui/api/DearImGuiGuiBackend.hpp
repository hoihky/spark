#pragma once

#include "spark/gui/api/IGuiBackend.hpp"
#include "spark/gui/api/IGuiFrame.hpp"

namespace Spark {

class IImGuiLayer;

namespace Gui {

class DearImGuiGuiFrame final : public IGuiFrame {
public:
    void Text(const char* text) override;
    void TextDisabled(const char* text) override;
    void Separator() override;
    bool Button(const char* id, const char* label) override;
    bool Checkbox(const char* id, const char* label, bool& value) override;
    bool SliderFloat(const char* id, const char* label, float& value, float minValue, float maxValue) override;
    bool DragFloat3(const char* id, const char* label, float values[3], float speed) override;
    bool Combo(const char* id, const char* label, int& currentItem, const char* const items[], int itemCount) override;
    bool BeginPanel(const char* id, const char* title, bool* open) override;
    void EndPanel() override;
    void SameLine(float offsetFromStartX, float spacing) override;
    bool Selectable(const char* id, const char* label, bool selected) override;
    bool MenuItem(const char* label, bool* checked) override;
    void SetCursorPos(float x, float y) override;
    void SetNextPanelSize(float width, float height) override;

    void ResetPanelStack() noexcept { imguiPanelStack = 0; }

private:
    int imguiPanelStack = 0;
};

class DearImGuiGuiBackend final : public IGuiBackend {
public:
    explicit DearImGuiGuiBackend(IImGuiLayer& layer) noexcept : imguiLayer(layer) {}

    [[nodiscard]] GuiBackendKind GetKind() const noexcept override { return GuiBackendKind::DearImGui; }

    void ProcessInput(
            GameWorld& world,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX,
            float contentScaleY) override;

    void Paint(
            const GameWorld& world,
            SceneRenderParams& params,
            int framebufferWidth,
            int framebufferHeight) override;

    void BeginImmediateFrame(const GuiFrameContext& context) override;
    void EndImmediateFrame() override;

    [[nodiscard]] IGuiFrame& GetImmediateFrame() noexcept override { return immediateFrame; }

    [[nodiscard]] bool WantsCaptureMouse() const noexcept override;
    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept override;

    void OnEnginePreRender(Window& window, IInput& input, float deltaTimeSeconds) override;
    void OnEnginePostRender() override;

private:
    IImGuiLayer& imguiLayer;
    DearImGuiGuiFrame immediateFrame;
};

}  // namespace Gui
}  // namespace Spark
