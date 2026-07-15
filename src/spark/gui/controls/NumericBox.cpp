#include "spark/gui/controls/NumericBox.hpp"

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Spark::Gui {

namespace {

Utf8String FormatNumber(const float v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.5g", static_cast<double>(v));
    return Utf8String(buf);
}

}  // namespace

NumericBox::NumericBox() = default;

void NumericBox::SetValue(const float v) noexcept {
    value = std::clamp(v, std::min(minV, maxV), std::max(minV, maxV));
}

void NumericBox::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.FillDropShadow(
            bounds.x, bounds.y, bounds.width, bounds.height, 2.5F, 3.0F, th.shadowRgb, 0.45F);
    ctx.FillRectGradientVertical(bounds.x, bounds.y, bounds.width, bounds.height, th.numericFillTop,
            th.numericFillBottom, 0.9F);
    const Vector3 border = dragging ? th.numericBorderDragging : th.numericBorderIdle;
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, border, dragging ? 0.85F : 0.5F);
    const Utf8String text = FormatNumber(value);
    const float textY = bounds.y + std::max(4.0F, (bounds.height - 20.0F) * 0.5F);
    ctx.DrawText(bounds.x + 8.0F, textY, 20.0F, text, th.labelPrimary, 1.0F);
    ctx.DrawText(bounds.x + bounds.width - 120.0F, textY, 14.0F,
            Utf8String("drag"), th.labelMuted, 0.85F);
}

void NumericBox::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !bounds.Contains(in.mouseX, in.mouseY)) {
        return;
    }
    dragging = true;
    anchorX = in.mouseX;
    anchorValue = value;
}

void NumericBox::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging) {
        return;
    }
    const float delta = in.mouseX - anchorX;
    const float next = anchorValue + delta * sensitivity;
    const float clamped = std::clamp(next, std::min(minV, maxV), std::max(minV, maxV));
    if (std::fabs(clamped - value) > 1.0e-6F) {
        value = clamped;
        if (onChanged) {
            onChanged(value);
        }
    }
}

void NumericBox::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    dragging = false;
}

}  // namespace Spark::Gui
