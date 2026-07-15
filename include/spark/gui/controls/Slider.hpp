#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;

/** Horizontal value slider between a min and max; drag anywhere on the track to move the thumb. */
class Slider final : public Widget {
public:
    Slider();
    void SetRange(float minValue, float maxValue) noexcept {
        minV = minValue;
        maxV = maxValue;
    }
    void SetValue(float v) noexcept;
    [[nodiscard]] float GetValue() const noexcept { return value; }
    void SetOnChanged(std::function<void(float)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Paint(GuiPaintContext& ctx) const override;
    void ProcessKeyInput(IInput& input) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }

private:
    void ApplyPointerX(float mx);

    float minV = 0.0F;
    float maxV = 1.0F;
    float value = 0.5F;
    bool dragging = false;
    std::function<void(float)> onChanged{};
};

}  // namespace Spark::Gui
