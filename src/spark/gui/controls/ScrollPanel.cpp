#include "spark/gui/controls/ScrollPanel.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

namespace {

/** Deepest Y among visible descendants (including <c>w</c>'s own rect). */
float MaxVisibleSubtreeBottom(const Widget* w) {
    if (w == nullptr || !w->IsVisible()) {
        return -1.0e30F;
    }
    float m = w->GetBounds().y + w->GetBounds().height;
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (!ch[i] || !ch[i]->IsVisible()) {
            continue;
        }
        m = std::max(m, MaxVisibleSubtreeBottom(ch[i].Get()));
    }
    return m;
}

}  // namespace

ScrollPanel::ScrollPanel() = default;

bool ScrollPanel::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

void ScrollPanel::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = std::max(0.0F, trackRect.height - thumbRect.height);
    if (travel <= 1.0e-4F) {
        return;
    }
    /** Thumb moves down (larger <c>thumbTopY</c>) → larger <c>t</c> → larger <c>scrollY</c> → rows use smaller screen Y → content moves up. */
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = std::clamp(t, 0.0F, 1.0F) * maxScroll;
}

void ScrollPanel::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

void ScrollPanel::ScrollToTop() noexcept {
    scrollY = 0.0F;
    SyncScrollLayout();
}

void ScrollPanel::Arrange(const Rect& r) {
    arrangeRect = r;
    bounds = r;
    if (!visible || r.height <= 0.0F || r.width <= 0.0F) {
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i]) {
                children[i]->Arrange({r.x, r.y, r.width, 0.0F});
            }
        }
        contentHeight = 0.0F;
        maxScroll = 0.0F;
        trackRect = {};
        thumbRect = {};
        return;
    }
    const float innerW = std::max(0.0F, r.width - kTrackW);
    const Rect inner{r.x, r.y, innerW, r.height};
    const float viewportTop = inner.y;

    const std::size_t n = children.GetSize();
    const bool useCustomHeights = (n > 0) && (perChildHeights.GetSize() == n);

    Array<float> effH;
    effH.Resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const float base = useCustomHeights ? std::max(4.0F, perChildHeights[i]) : rowHeight;
        effH[i] = base;
    }

    /** Measure each row at <c>inner.y + y</c> (no scroll) so we learn true stacked height including pop-outs. */
    float yMeasure = 0.0F;
    for (std::size_t i = 0; i < n; ++i) {
        if (!children[i] || !children[i]->IsVisible()) {
            effH[i] = 0.0F;
            continue;
        }
        children[i]->Arrange({inner.x, inner.y + yMeasure, innerW, effH[i]});
        const float top = children[i]->GetBounds().y;
        const float deep = MaxVisibleSubtreeBottom(children[i].Get());
        effH[i] = std::max(effH[i], std::max(4.0F, deep - top));
        yMeasure += effH[i];
        if (i + 1 < n) {
            yMeasure += vGap;
        }
    }

    contentHeight = 0.0F;
    for (std::size_t i = 0; i < n; ++i) {
        if (effH[i] <= 0.0F) {
            continue;
        }
        contentHeight += effH[i];
        if (i + 1 < n) {
            contentHeight += vGap;
        }
    }

    maxScroll = std::max(0.0F, contentHeight - inner.height);
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);

    float y = 0.0F;
    for (std::size_t i = 0; i < n; ++i) {
        if (!children[i] || !children[i]->IsVisible() || effH[i] <= 0.0F) {
            continue;
        }
        /** Outer rect stays <c>bounds</c>; only content is translated by <c>scrollY</c> inside the viewport. */
        children[i]->Arrange({inner.x, viewportTop - scrollY + y, innerW, effH[i]});
        y += effH[i];
        if (i + 1 < n) {
            y += vGap;
        }
    }

    trackRect = {r.x + innerW, r.y, kTrackW, r.height};
    if (maxScroll <= 0.0F || contentHeight <= 0.0F) {
        thumbRect = {trackRect.x + 1.0F, trackRect.y + 1.0F, trackRect.width - 2.0F, trackRect.height - 2.0F};
    } else {
        const float minThumb = 22.0F;
        const float thumbH =
                std::max(minThumb, inner.height * (inner.height / std::max(contentHeight, inner.height)));
        const float travel = std::max(0.0F, trackRect.height - thumbH);
        const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
        thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
    }
}

void ScrollPanel::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const float innerW = std::max(0.0F, bounds.width - kTrackW);
    /** Pixel-aligned clip so Vulkan scissor matches layout coords and leaks are avoided at edges. */
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    const float clipW = std::max(1.0F, clipR - clipX);
    const float clipH = std::max(1.0F, clipB - clipY);

    ctx.PushClipRect(clipX, clipY, clipW, clipH);
    const GuiTheme& th = ctx.GetTheme();
    if (useCustomViewportFill) {
        ctx.FillRectGradientVertical(
                bounds.x,
                bounds.y,
                innerW,
                bounds.height,
                customViewportTop,
                customViewportBottom,
                customViewportAlpha);
    } else {
        ctx.FillRectGradientVertical(bounds.x, bounds.y, innerW, bounds.height, th.scrollViewportTop,
                th.scrollViewportBottom, th.scrollViewportAlpha);
    }
    ctx.StrokeRect(bounds.x, bounds.y, innerW, bounds.height, 1.0F, th.borderRgb, 0.62F);
    PaintChildren(ctx);
    ctx.PopClipRect();

    ctx.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, th.insetTrackRgb, 0.92F);
    ctx.FillRectGradientVertical(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, th.thumbGradientTop,
            th.thumbGradientBottom, 0.95F);
    ctx.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, th.borderRgb, 0.5F);
}

Widget* ScrollPanel::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (HitTrack(x, y)) {
        return this;
    }
    const float innerW = std::max(0.0F, bounds.width - kTrackW);
    const Rect content{bounds.x, bounds.y, innerW, bounds.height};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    const auto& ch = GetChildren();
    for (std::size_t i = ch.GetSize(); i > 0; --i) {
        Widget* c = ch[i - 1U].Get();
        if (c == nullptr || !c->IsVisible()) {
            continue;
        }
        const Rect& b = c->GetBounds();
        if (b.y + b.height <= bounds.y || b.y >= bounds.y + bounds.height) {
            continue;
        }
        if (Widget* hit = c->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (hitTest && content.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void ScrollPanel::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    if (HitTrack(in.mouseX, in.mouseY)) {
        if (thumbRect.Contains(in.mouseX, in.mouseY)) {
            draggingThumb = true;
            grabOffsetY = in.mouseY - thumbRect.y;
        } else {
            draggingThumb = true;
            grabOffsetY = thumbRect.height * 0.5F;
            UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
        }
        SyncScrollLayout();
    }
}

void ScrollPanel::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void ScrollPanel::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    draggingThumb = false;
}

void ScrollPanel::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    static constexpr float kPixelsPerWheelUnit = 48.0F;
    scrollY = std::clamp(scrollY - deltaY * kPixelsPerWheelUnit, 0.0F, maxScroll);
    SyncScrollLayout();
}

}  // namespace Spark::Gui
