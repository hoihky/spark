#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Displays a numeric value; click-drag horizontally to change (sensitivity in units per pixel).
 */
class NumericBox final : public Widget {
public:
    NumericBox();
    void SetRange(float minValue, float maxValue) noexcept {
        minV = minValue;
        maxV = maxValue;
    }
    void SetSensitivity(float unitsPerPixel) noexcept { sensitivity = unitsPerPixel; }
    void SetValue(float v) noexcept;
    [[nodiscard]] float GetValue() const noexcept { return value; }
    void SetOnChanged(std::function<void(float)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    float minV = -100.0F;
    float maxV = 100.0F;
    float value = 0.0F;
    float sensitivity = 0.12F;
    bool dragging = false;
    float anchorX = 0.0F;
    float anchorValue = 0.0F;
    std::function<void(float)> onChanged{};
};

}  // namespace Spark::Gui
