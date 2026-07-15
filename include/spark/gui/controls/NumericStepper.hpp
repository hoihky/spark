#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {
class IInput;
}

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Compact numeric editor: − / + buttons and a center value (drag horizontally like <c>NumericBox</c>).
 * Intended for tile indices, zoom, grid size, etc.
 */
class NumericStepper final : public Widget {
public:
    NumericStepper();
    void SetRange(float minValue, float maxValue) noexcept {
        minV = minValue;
        maxV = maxValue;
    }
    void SetStep(float s) noexcept { step = s > 0.0F ? s : 1.0F; }
    void SetSensitivity(float unitsPerPixel) noexcept { sensitivity = unitsPerPixel; }
    /** When true, focused stepper accepts 0–9 to build integers and Backspace to truncate (step must be ≥ 1). */
    void SetAllowTypedIntegerEntry(bool allow) noexcept { allowTypedIntegerEntry = allow; }
    void SetValue(float v) noexcept;
    [[nodiscard]] float GetValue() const noexcept { return value; }
    void SetOnChanged(std::function<void(float)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;

private:
    enum class DragZone : unsigned char { None, Center, LeftBtn, RightBtn };

    void ApplyValue(float v);
    void BumpBySteps(int steps);

    float minV = 0.0F;
    float maxV = 4096.0F;
    float step = 1.0F;
    float value = 0.0F;
    float sensitivity = 0.1F;
    bool dragging = false;
    float anchorX = 0.0F;
    float anchorValue = 0.0F;
    DragZone pendingZone = DragZone::None;
    std::function<void(float)> onChanged{};
    bool allowTypedIntegerEntry = false;
};

}  // namespace Spark::Gui
