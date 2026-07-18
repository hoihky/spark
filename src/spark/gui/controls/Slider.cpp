#include "spark/gui/controls/Slider.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

Slider::Slider() = default;

void Slider::SetValue(const float v) noexcept {
    value = std::clamp(v, std::min(minV, maxV), std::max(minV, maxV));
}

void Slider::ApplyPointerX(const float mx) {
    const float lo = std::min(minV, maxV);
    const float hi = std::max(minV, maxV);
    const float span = std::max(0.001F, bounds.width);
    const float t = std::clamp((mx - bounds.x) / span, 0.0F, 1.0F);
    const float next = lo + t * (hi - lo);
    if (std::fabs(next - value) > 1.0e-5F) {
        value = next;
        if (onChanged) {
            onChanged(value);
        }
    }
}

void Slider::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float h = bounds.height;
    const float trackY = bounds.y + h * 0.5F - 3.0F;
    ctx.FillRect(bounds.x, trackY, bounds.width, 6.0F, th.sliderTrackRgb, 0.65F);
    const float lo = std::min(minV, maxV);
    const float hi = std::max(minV, maxV);
    const float t = hi > lo ? (value - lo) / (hi - lo) : 0.0F;
    const float thumbX = bounds.x + t * bounds.width - 8.0F;
    const float thumbY = bounds.y + h * 0.5F - 10.0F;
    ctx.FillDropShadow(thumbX, thumbY, 16.0F, 20.0F, 1.5F, 2.0F, th.shadowRgb, 0.55F);
    ctx.FillRectGradientVertical(
            thumbX, thumbY, 16.0F, 20.0F, th.sliderThumbTop, th.sliderThumbBottom, 0.95F);
    ctx.StrokeRect(thumbX, thumbY, 16.0F, 20.0F, 1.0F, th.borderRgb, 0.55F);
}

void Slider::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !bounds.Contains(in.mouseX, in.mouseY)) {
        return;
    }
    dragging = true;
    ApplyPointerX(in.mouseX);
}

void Slider::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging) {
        return;
    }
    ApplyPointerX(in.mouseX);
}

void Slider::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    dragging = false;
}

void Slider::ProcessKeyInput(IInput& input) {
    if (!enabled) {
        return;
    }
    const float lo = std::min(minV, maxV);
    const float hi = std::max(minV, maxV);
    const float span = std::max(0.001F, hi - lo);
    const float step = span * 0.02F;
    float next = value;
    if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT) || input.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
        next = value - step;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT) || input.IsKeyPressedThisFrame(GLFW_KEY_UP)) {
        next = value + step;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        next = lo;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_END)) {
        next = hi;
    } else {
        return;
    }
    SetValue(next);
    if (onChanged) {
        onChanged(value);
    }
}

}  // namespace Spark::Gui
