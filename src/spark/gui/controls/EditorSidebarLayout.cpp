#include "spark/gui/controls/EditorSidebarLayout.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

void EditorSidebarLayout::Arrange(const Rect& r) {
    bounds = r;
    const auto& ch = GetChildren();
    if (ch.GetSize() < 2U) {
        Widget::Arrange(r);
        gutterRect = {0.0F, 0.0F, 0.0F, 0.0F};
        return;
    }
    const float g2 = gutterHalf * 2.0F;
    const float maxSidebar = std::max(160.0F, r.width * 0.5F - g2);
    const float sw = std::clamp(sidebarWidthPx, 160.0F, maxSidebar);
    sidebarWidthPx = sw;
    if (ch[0]) {
        ch[0]->Arrange({r.x, r.y, sw, r.height});
    }
    const float rightX = r.x + sw + g2;
    if (ch[1]) {
        ch[1]->SetHitTest(false);
        ch[1]->Arrange({rightX, r.y, std::max(0.0F, r.width - sw - g2), r.height});
    }
    gutterRect = {r.x + sw, r.y, g2, r.height};
}

void EditorSidebarLayout::Paint(GuiPaintContext& ctx) const {
    PaintChildren(ctx);
    if (gutterRect.width <= 0.0F) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.PushOverlayLayer();
    ctx.FillRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, th.insetTrackRgb, 1.0F);
    ctx.StrokeRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, 1.0F, th.borderRgb, 0.65F);
    ctx.PopOverlayLayer();
}

Widget* EditorSidebarLayout::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (HitGutter(x, y)) {
        return this;
    }
    const auto& ch = GetChildren();
    if (ch.GetSize() >= 2U && ch[1]) {
        if (Widget* hit = ch[1]->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (ch.GetSize() >= 1U && ch[0]) {
        if (Widget* hit = ch[0]->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

bool EditorSidebarLayout::HitGutter(const float x, const float y) const noexcept {
    return gutterRect.width > 0.0F && gutterRect.Contains(x, y);
}

void EditorSidebarLayout::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    if (HitGutter(in.mouseX, in.mouseY)) {
        dragging = true;
    }
}

void EditorSidebarLayout::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging) {
        return;
    }
    const float g2 = gutterHalf * 2.0F;
    const float maxSidebar = std::max(160.0F, bounds.width * 0.5F - g2);
    sidebarWidthPx = std::clamp(in.mouseX - bounds.x - gutterHalf, 160.0F, maxSidebar);
    if (onSidebarWidthChanged) {
        onSidebarWidthChanged(sidebarWidthPx, false);
    }
}

void EditorSidebarLayout::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    if (dragging && onSidebarWidthChanged) {
        onSidebarWidthChanged(sidebarWidthPx, true);
    }
    dragging = false;
}

}  // namespace Spark::Gui
