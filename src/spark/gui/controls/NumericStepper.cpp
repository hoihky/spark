#include "spark/gui/controls/NumericStepper.hpp"

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <GLFW/glfw3.h>

namespace Spark::Gui {

namespace {

Utf8String FormatNumber(const float v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.5g", static_cast<double>(v));
    return Utf8String(buf);
}

}  // namespace

NumericStepper::NumericStepper() = default;

void NumericStepper::SetValue(const float v) noexcept {
    value = std::clamp(v, (std::min)(minV, maxV), (std::max)(minV, maxV));
}

void NumericStepper::ApplyValue(const float v) {
    const float c = std::clamp(v, (std::min)(minV, maxV), (std::max)(minV, maxV));
    if (std::fabs(c - value) > 1.0e-6F) {
        value = c;
        if (onChanged) {
            onChanged(value);
        }
    }
}

void NumericStepper::BumpBySteps(const int steps) {
    if (steps == 0) {
        return;
    }
    ApplyValue(value + static_cast<float>(steps) * step);
}

void NumericStepper::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float side = (std::min)(40.0F, (std::max)(28.0F, bounds.width * 0.18F));
    const float xL = bounds.x;
    const float xR = bounds.x + bounds.width - side;
    const float midX = xL + side;
    const float midW = (std::max)(0.0F, bounds.width - 2.0F * side);

    ctx.FillDropShadow(bounds.x, bounds.y, bounds.width, bounds.height, 2.0F, 2.5F, th.shadowRgb, 0.4F);
    ctx.FillRectGradientVertical(xL, bounds.y, side, bounds.height, th.controlIdleTop, th.controlIdleBottom, 0.9F);
    ctx.FillRectGradientVertical(xR, bounds.y, side, bounds.height, th.controlIdleTop, th.controlIdleBottom, 0.9F);
    ctx.FillRectGradientVertical(midX, bounds.y, midW, bounds.height, th.numericFillTop, th.numericFillBottom, 0.92F);

    const Vector3 border = dragging ? th.numericBorderDragging : th.numericBorderIdle;
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, border, dragging ? 0.8F : 0.5F);

    const float ty = bounds.y + (std::max)(4.0F, (bounds.height - 20.0F) * 0.5F);
    ctx.DrawText(xL + (std::max)(4.0F, (side - 14.0F) * 0.5F), ty, 22.0F, Utf8String("-"), th.labelPrimary, 1.0F);
    ctx.DrawText(xR + (std::max)(4.0F, (side - 14.0F) * 0.5F), ty, 22.0F, Utf8String("+"), th.labelPrimary, 1.0F);

    const Utf8String text = FormatNumber(value);
    ctx.DrawText(midX + 8.0F, ty, 20.0F, text, th.labelPrimary, 1.0F);
}

void NumericStepper::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !bounds.Contains(in.mouseX, in.mouseY)) {
        pendingZone = DragZone::None;
        return;
    }
    const float side = (std::min)(40.0F, (std::max)(28.0F, bounds.width * 0.18F));
    const float xR = bounds.x + bounds.width - side;
    if (in.mouseX < bounds.x + side) {
        pendingZone = DragZone::LeftBtn;
        BumpBySteps(-1);
        return;
    }
    if (in.mouseX >= xR) {
        pendingZone = DragZone::RightBtn;
        BumpBySteps(1);
        return;
    }
    pendingZone = DragZone::Center;
    dragging = true;
    anchorX = in.mouseX;
    anchorValue = value;
}

void NumericStepper::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging || pendingZone != DragZone::Center) {
        return;
    }
    const float delta = in.mouseX - anchorX;
    const float next = anchorValue + delta * sensitivity;
    ApplyValue(next);
}

void NumericStepper::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    dragging = false;
    pendingZone = DragZone::None;
}

void NumericStepper::ProcessKeyInput(IInput& input) {
    if (!enabled) {
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_UP) || input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
        BumpBySteps(1);
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_DOWN) || input.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
        BumpBySteps(-1);
        return;
    }
    if (allowTypedIntegerEntry && step >= 1.0F) {
        for (int k = GLFW_KEY_0; k <= GLFW_KEY_9; ++k) {
            if (input.IsKeyPressedThisFrame(k)) {
                const float d = static_cast<float>(k - GLFW_KEY_0);
                const float cur = std::round(value);
                const float next = cur * 10.0F + d;
                ApplyValue(next);
                return;
            }
        }
        if (input.IsKeyPressedThisFrame(GLFW_KEY_BACKSPACE)) {
            const float cur = std::round(value);
            ApplyValue(std::trunc(cur / 10.0F));
        }
    }
}

}  // namespace Spark::Gui
