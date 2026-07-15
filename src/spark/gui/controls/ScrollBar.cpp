#include "spark/gui/controls/ScrollBar.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

ScrollBar::ScrollBar() = default;

void ScrollBar::Arrange(const Rect& r) {
    bounds = r;
    const bool vert = axis == ScrollBarAxis::Vertical;
    trackLen = vert ? r.height : r.width;
    thumbLen = std::max(18.0F, trackLen * 0.22F);
    const float span = std::max(0.0F, trackLen - thumbLen);
    thumbStart = value01 * span;
}

void ScrollBar::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& t = ctx.GetTheme();
    ctx.FillRect(bounds.x, bounds.y, bounds.width, bounds.height, t.insetTrackRgb, 0.55F);
    const bool vert = axis == ScrollBarAxis::Vertical;
    float tx = bounds.x;
    float ty = bounds.y;
    float tw = bounds.width;
    float th = bounds.height;
    if (vert) {
        ty += thumbStart;
        th = thumbLen;
    } else {
        tx += thumbStart;
        tw = thumbLen;
    }
    ctx.FillRectGradientVertical(tx, ty, tw, th, t.thumbGradientTop, t.thumbGradientBottom, 0.92F);
    ctx.StrokeRect(tx, ty, tw, th, 1.0F, t.borderRgb, 0.5F);
}

void ScrollBar::SyncFromPointer(const float x, const float y) {
    const bool vert = axis == ScrollBarAxis::Vertical;
    const float span = std::max(0.001F, trackLen - thumbLen);
    const float pos = vert ? (y - bounds.y - grabAlong) : (x - bounds.x - grabAlong);
    const float next = pos / span;
    const float clamped = std::clamp(next, 0.0F, 1.0F);
    if (clamped != value01) {
        value01 = clamped;
        if (onChanged) {
            onChanged(value01);
        }
    }
    const float span2 = std::max(0.0F, trackLen - thumbLen);
    thumbStart = value01 * span2;
}

void ScrollBar::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !bounds.Contains(in.mouseX, in.mouseY)) {
        return;
    }
    dragging = true;
    const bool vert = axis == ScrollBarAxis::Vertical;
    const float pos = vert ? (in.mouseY - bounds.y) : (in.mouseX - bounds.x);
    grabAlong = pos - thumbStart;
    if (grabAlong < 0.0F || grabAlong > thumbLen) {
        grabAlong = thumbLen * 0.5F;
        SyncFromPointer(in.mouseX, in.mouseY);
    }
}

void ScrollBar::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging) {
        return;
    }
    SyncFromPointer(in.mouseX, in.mouseY);
}

void ScrollBar::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    dragging = false;
}

}  // namespace Spark::Gui
