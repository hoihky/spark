#include "spark/gui/controls/List.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/controls/Button.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

void List::SetItemBold(const bool b) noexcept {
    itemBold = b;
    if (!items.IsEmpty()) {
        Rebuild();
    }
}

void List::SetOpaqueRows(const bool v) noexcept {
    opaqueRows = v;
    if (!items.IsEmpty()) {
        Rebuild();
    }
}

void List::Rebuild() {
    ClearChildren();
    for (std::size_t i = 0; i < items.GetSize(); ++i) {
        auto b = MakeUnique<Button>();
        Button* raw = b.Get();
        raw->SetLabel(items[i]);
        raw->SetFontSize(itemFontPx);
        raw->SetLabelBold(itemBold);
        raw->SetOpaqueSurface(opaqueRows);
        const int idx = static_cast<int>(i);
        raw->SetOnClick([this, idx]() {
            selectedIndex = idx;
            if (onSelect) {
                onSelect(idx);
            }
        });
        AddChild(Spark::MoveTemp(b));
    }
}

void List::SetItems(Array<Utf8String> lines) {
    items = Spark::MoveTemp(lines);
    scrollY = 0.0F;
    Rebuild();
}

void List::SetSelectedIndex(const int i) noexcept {
    if (items.IsEmpty() || children.IsEmpty()) {
        selectedIndex = -1;
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    selectedIndex = std::clamp(i, 0, n - 1);
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (Button* b = dynamic_cast<Button*>(children[j].Get())) {
            b->SetAccentSelected(static_cast<int>(j) == selectedIndex);
        }
    }
}

List::List() = default;

void List::SetVerticalScrollingEnabled(const bool enabled) noexcept {
    verticalScrollingEnabled = enabled;
    if (!enabled) {
        scrollY = 0.0F;
        draggingThumb = false;
    }
}

bool List::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

bool List::RowIntersectsViewport(const Rect& rowBounds) const noexcept {
    if (maxScroll <= 0.0F) {
        return true;
    }
    const float overflowTop = std::min(scrollY, bounds.y);
    const float viewTop = bounds.y - overflowTop;
    const float viewBottom = bounds.y + bounds.height;
    return rowBounds.y + rowBounds.height > viewTop && rowBounds.y < viewBottom;
}

void List::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

void List::ScrollToTop() noexcept {
    scrollY = 0.0F;
    SyncScrollLayout();
}

void List::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = std::max(0.0F, trackRect.height - thumbRect.height);
    if (travel <= 1.0e-4F) {
        return;
    }
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = std::clamp(t, 0.0F, 1.0F) * maxScroll;
}

void List::SnapScrollToRowGrid(const float scaledRowH) noexcept {
    if (scaledRowH < 0.5F) {
        return;
    }
    if (scrollY < 1.0F) {
        scrollY = 0.0F;
        return;
    }
    if (scrollY <= scaledRowH * 0.5F) {
        scrollY = 0.0F;
    } else {
        scrollY = std::floor(scrollY / scaledRowH + 0.5F) * scaledRowH;
    }
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);
}

void List::Arrange(const Rect& r) {
    arrangeRect = r;
    bounds = r;
    if (!visible || r.width < 0.5F || r.height < 0.5F) {
        maxScroll = 0.0F;
        scrollY = 0.0F;
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i]) {
                children[i]->Arrange({0.0F, 0.0F, 0.0F, 0.0F});
            }
        }
        return;
    }
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float scaledRowH = m.Scaled(rowHeight);
    const float trackW = m.Scaled(kTrackW);
    const std::size_t n = children.GetSize();
    contentHeight = scaledRowH * static_cast<float>(n);

    if (!verticalScrollingEnabled) {
        scrollY = 0.0F;
        maxScroll = 0.0F;
        bounds.height = std::max(r.height, contentHeight);
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
        for (std::size_t i = 0; i < n; ++i) {
            if (Button* b = dynamic_cast<Button*>(children[i].Get())) {
                b->SetAccentSelected(static_cast<int>(i) == selectedIndex);
            }
            if (children[i]) {
                children[i]->Arrange(
                        {r.x, r.y + static_cast<float>(i) * scaledRowH, r.width, scaledRowH});
            }
        }
        return;
    }

    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - trackW) : r.width;
    const float viewportTop = r.y;

    for (std::size_t i = 0; i < n; ++i) {
        if (Button* b = dynamic_cast<Button*>(children[i].Get())) {
            b->SetAccentSelected(static_cast<int>(i) == selectedIndex);
        }
        if (children[i]) {
            const float y = viewportTop - scrollY + static_cast<float>(i) * scaledRowH;
            children[i]->Arrange({r.x, y, innerW, scaledRowH});
        }
    }

    if (maxScroll <= 0.0F) {
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
    } else {
        trackRect = {r.x + innerW, r.y, trackW, r.height};
        if (contentHeight <= 0.0F) {
            thumbRect = {trackRect.x + 1.0F, trackRect.y + 1.0F, trackRect.width - 2.0F, trackRect.height - 2.0F};
        } else {
            const float kMinThumb = m.Scaled(22.0F);
            const float thumbH =
                    std::max(kMinThumb, r.height * (r.height / std::max(contentHeight, r.height)));
            const float travel = std::max(0.0F, trackRect.height - thumbH);
            const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
            thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
        }
    }
}

void List::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    if (bounds.height < 0.5F || bounds.width < 0.5F) {
        return;
    }
    if (maxScroll <= 0.0F) {
        Widget::Paint(ctx);
        return;
    }

    const float trackW = ctx.GetLayoutMetrics().Scaled(kTrackW);
    const float innerW = std::max(0.0F, bounds.width - (maxScroll > 0.0F ? trackW : 0.0F));
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    const float clipW = std::max(1.0F, clipR - clipX);
    const float clipH = std::max(1.0F, clipB - clipY);
    /** Rows scroll above <c>bounds.y</c>; extend fill/clip so they are not clipped to black. */
    const float overflowTop = std::min(scrollY, clipY);
    const float fillY = clipY - overflowTop;
    const float fillH = clipH + overflowTop;

    const GuiTheme& th = ctx.GetTheme();
    const float viewportAlpha = opaqueViewport ? 1.0F : th.scrollViewportAlpha;
    ctx.FillRectGradientVertical(
            clipX, fillY, clipW, fillH, th.scrollViewportTop, th.scrollViewportBottom, viewportAlpha);

    ctx.PushClipRect(clipX, fillY, clipW, fillH);
    const auto& ch = GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (!ch[i] || !ch[i]->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(ch[i]->GetBounds())) {
            continue;
        }
        ch[i]->Paint(ctx);
    }
    ctx.PopClipRect();

    ctx.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, th.insetTrackRgb, 0.92F);
    ctx.FillRectGradientVertical(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, th.thumbGradientTop,
            th.thumbGradientBottom, 0.95F);
    ctx.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, th.borderRgb, 0.5F);
}

Widget* List::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled || bounds.height < 0.5F || bounds.width < 0.5F) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float trackW = m.Scaled(kTrackW);
    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, bounds.width - trackW) : bounds.width;
    const float overflowTop = std::min(scrollY, bounds.y);
    const Rect content{bounds.x, bounds.y - overflowTop, innerW, bounds.height + overflowTop};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    const auto& ch = GetChildren();
    for (std::size_t i = ch.GetSize(); i > 0; --i) {
        Widget* c = ch[i - 1U].Get();
        if (c == nullptr || !c->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(c->GetBounds())) {
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

void List::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
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

void List::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void List::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    if (draggingThumb) {
        const float scaledRowH = GetActiveGuiLayoutMetrics().Scaled(rowHeight);
        SnapScrollToRowGrid(scaledRowH);
        SyncScrollLayout();
    }
    draggingThumb = false;
}

void List::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    const float scaledRowH = GetActiveGuiLayoutMetrics().Scaled(rowHeight);
    const int steps = static_cast<int>(std::round(deltaY));
    if (steps == 0) {
        return;
    }
    scrollY = std::clamp(scrollY - static_cast<float>(steps) * scaledRowH, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);
    SyncScrollLayout();
}

void List::ScrollSelectedIndexIntoView() noexcept {
    if (!verticalScrollingEnabled || maxScroll <= 0.0F || selectedIndex < 0) {
        return;
    }
    const int idx = selectedIndex;
    const float scaledRowH = GetActiveGuiLayoutMetrics().Scaled(rowHeight);
    const float rowTop = static_cast<float>(idx) * scaledRowH;
    const float rowBottom = rowTop + scaledRowH;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);
    SyncScrollLayout();
}

void List::ProcessKeyInput(IInput& input) {
    if (!enabled || items.IsEmpty()) {
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    int cur = selectedIndex >= 0 ? selectedIndex : 0;
    cur = std::clamp(cur, 0, n - 1);
    int next = cur;
    if (input.IsKeyPressedThisFrame(GLFW_KEY_UP)) {
        next = std::max(0, cur - 1);
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
        next = std::min(n - 1, cur + 1);
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        next = 0;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_END)) {
        next = n - 1;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_ENTER)) {
        if (selectedIndex >= 0 && onSelect) {
            onSelect(selectedIndex);
        }
        return;
    } else {
        return;
    }
    SetSelectedIndex(next);
    if (onSelect) {
        onSelect(next);
    }
    ScrollSelectedIndexIntoView();
}

}  // namespace Spark::Gui
