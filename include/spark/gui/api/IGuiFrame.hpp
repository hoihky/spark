#pragma once

#include <cstddef>

namespace Spark::Gui {

/**
 * Portable immediate-mode control surface (Dependency Inversion).
 * Game code depends on this interface; backends implement it with Spark widgets or Dear ImGui.
 */
class IGuiFrame {
public:
    virtual ~IGuiFrame() = default;

    virtual void Text(const char* text) = 0;
    virtual void TextDisabled(const char* text) = 0;
    virtual void Separator() = 0;

    /** @return true when activated this frame (pressed and released over the control). */
    virtual bool Button(const char* id, const char* label) = 0;

    virtual bool Checkbox(const char* id, const char* label, bool& value) = 0;
    virtual bool SliderFloat(const char* id, const char* label, float& value, float minValue, float maxValue) = 0;
    virtual bool DragFloat3(const char* id, const char* label, float values[3], float speed = 0.05F) = 0;
    virtual bool Combo(const char* id, const char* label, int& currentItem, const char* const items[], int itemCount) = 0;

    virtual bool BeginPanel(const char* id, const char* title, bool* open = nullptr) = 0;
    virtual void EndPanel() = 0;

    virtual void SameLine(float offsetFromStartX = 0.0F, float spacing = -1.0F) = 0;

    /** @return true when the item was clicked this frame. */
    virtual bool Selectable(const char* id, const char* label, bool selected) = 0;

    virtual bool MenuItem(const char* label, bool* checked = nullptr) = 0;

    virtual void SetCursorPos(float x, float y) = 0;

    /** Optional size for the next root panel (Spark native immediate UI). */
    virtual void SetNextPanelSize(float width, float height) {
        (void)width;
        (void)height;
    }
};

}  // namespace Spark::Gui
