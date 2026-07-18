#include "spark/gui/controls/Splitter.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

void Splitter::Arrange(const Rect& r) {
    bounds = r;
    const auto& ch = GetChildren();
    if (ch.IsEmpty()) {
        return;
    }
    if (ch.GetSize() == 1U) {
        if (ch[0]) {
            ch[0]->Arrange(r);
        }
        return;
    }
    const float g2 = gutterHalf * 2.0F;
    if (orientation == SplitterOrientation::Horizontal) {
        const float inner = std::max(0.0F, r.width - g2);
        const float w0 = inner * split;
        const float w1 = std::max(0.0F, inner - w0);
        if (ch[0]) {
            ch[0]->Arrange({r.x, r.y, w0, r.height});
        }
        if (ch[1]) {
            ch[1]->Arrange({r.x + w0 + g2, r.y, w1, r.height});
        }
    } else {
        const float inner = std::max(0.0F, r.height - g2);
        const float h0 = inner * split;
        const float h1 = std::max(0.0F, inner - h0);
        if (ch[0]) {
            ch[0]->Arrange({r.x, r.y, r.width, h0});
        }
        if (ch[1]) {
            ch[1]->Arrange({r.x, r.y + h0 + g2, r.width, h1});
        }
    }
}

void Splitter::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const auto& ch = GetChildren();
    if (ch.GetSize() < 2U) {
        Widget::Paint(ctx);
        return;
    }
    Widget::Paint(ctx);
    const GuiTheme& th = ctx.GetTheme();
    ctx.PushOverlayLayer();
    if (orientation == SplitterOrientation::Horizontal) {
        const float g2 = gutterHalf * 2.0F;
        const float inner = std::max(0.0F, bounds.width - g2);
        const float w0 = inner * split;
        const float gx = bounds.x + w0;
        ctx.FillRect(gx, bounds.y, g2, bounds.height, th.insetTrackRgb, 1.0F);
        ctx.StrokeRect(gx, bounds.y, g2, bounds.height, 1.0F, th.borderRgb, 0.65F);
    } else {
        const float g2 = gutterHalf * 2.0F;
        const float inner = std::max(0.0F, bounds.height - g2);
        const float h0 = inner * split;
        const float gy = bounds.y + h0;
        ctx.FillRect(bounds.x, gy, bounds.width, g2, th.insetTrackRgb, 1.0F);
        ctx.StrokeRect(bounds.x, gy, bounds.width, g2, 1.0F, th.borderRgb, 0.65F);
    }
    ctx.PopOverlayLayer();
}

bool Splitter::HitGutter(const float x, const float y) const noexcept {
    const auto& ch = GetChildren();
    if (ch.GetSize() < 2U) {
        return false;
    }
    const float g2 = gutterHalf * 2.0F;
    if (orientation == SplitterOrientation::Horizontal) {
        const float inner = std::max(0.0F, bounds.width - g2);
        const float w0 = inner * split;
        const Rect g{bounds.x + w0, bounds.y, g2, bounds.height};
        return g.Contains(x, y);
    }
    const float inner = std::max(0.0F, bounds.height - g2);
    const float h0 = inner * split;
    const Rect g{bounds.x, bounds.y + h0, bounds.width, g2};
    return g.Contains(x, y);
}

Widget* Splitter::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (HitGutter(x, y)) {
        return this;
    }
    // Do not claim the full splitter bounds — only the gutter is interactive. Otherwise the empty
    // viewport pane (hitTest=false child) would still leave this widget as the hover target and block 3D input.
    const auto& ch = GetChildren();
    for (std::size_t i = ch.GetSize(); i > 0U; --i) {
        if (Widget* hit = ch[i - 1U]->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

void Splitter::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    if (HitGutter(in.mouseX, in.mouseY)) {
        dragging = true;
    }
}

void Splitter::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !dragging) {
        return;
    }
    const float g2 = gutterHalf * 2.0F;
    if (orientation == SplitterOrientation::Horizontal) {
        const float inner = std::max(1.0F, bounds.width - g2);
        const float rel = in.mouseX - bounds.x - gutterHalf;
        SetSplit(rel / inner);
    } else {
        const float inner = std::max(1.0F, bounds.height - g2);
        const float rel = in.mouseY - bounds.y - gutterHalf;
        SetSplit(rel / inner);
    }
    if (onSplitChanged) {
        onSplitChanged(split, false);
    }
}

void Splitter::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    if (dragging && onSplitChanged) {
        onSplitChanged(split, true);
    }
    dragging = false;
}

}  // namespace Spark::Gui
