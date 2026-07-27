#pragma once

#include "spark/gui/api/GuiFrameContext.hpp"
#include "spark/gui/api/IGuiFrame.hpp"

namespace Spark::Gui {

class SparkNativeImmediateGuiFrame final : public IGuiFrame {
public:
    void ResetForFrame() noexcept;

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

    void BindContext(const GuiFrameContext* context) noexcept { frameContext = context; }

private:
    const GuiFrameContext* frameContext = nullptr;
    float cursorX = 16.0F;
    float cursorY = 16.0F;
    float lineHeight = 22.0F;
    float contentWidth = 320.0F;
    int panelDepth = 0;
    bool sameLine = false;
    float sameLineY = 0.0F;
    float panelOuterX = 0.0F;
    float panelOuterY = 0.0F;
    float panelOuterW = 0.0F;
    float panelOuterH = 0.0F;
    float pendingPanelW = 0.0F;
    float pendingPanelH = 0.0F;
    float contentStartX = 16.0F;
    float lineCursorX = 16.0F;
};

}  // namespace Spark::Gui
