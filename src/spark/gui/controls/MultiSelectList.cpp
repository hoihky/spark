#include "spark/gui/controls/MultiSelectList.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/controls/Button.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

void MultiSelectList::SetItemBold(const bool b) noexcept {
    itemBold = b;
    if (!items.IsEmpty()) {
        Rebuild();
    }
}

void MultiSelectList::SetOpaqueRows(const bool v) noexcept {
    opaqueRows = v;
    if (!items.IsEmpty()) {
        Rebuild();
    }
}

void MultiSelectList::SortUniqueSelected() {
    if (selected.IsEmpty()) {
        return;
    }
    const int n = VisibleRowCount();
    for (std::size_t i = 0; i < selected.GetSize(); ++i) {
        selected[i] = std::clamp(selected[i], 0, std::max(0, n - 1));
    }
    std::sort(selected.GetData(), selected.GetData() + selected.GetSize());
    std::size_t w = 0;
    for (std::size_t r = 0; r < selected.GetSize(); ++r) {
        if (w == 0 || selected[r] != selected[w - 1U]) {
            selected[w] = selected[r];
            ++w;
        }
    }
    while (selected.GetSize() > w) {
        selected.PopBack();
    }
}

bool MultiSelectList::IsSelected(const int idx) const noexcept {
    for (std::size_t i = 0; i < selected.GetSize(); ++i) {
        if (selected[i] == idx) {
            return true;
        }
    }
    return false;
}

void MultiSelectList::SetRowSelected(const int idx, const bool sel) {
    if (sel) {
        if (!IsSelected(idx)) {
            selected.PushBack(idx);
        }
    } else {
        for (std::size_t i = 0; i < selected.GetSize(); ++i) {
            if (selected[i] == idx) {
                selected.RemoveAt(i);
                break;
            }
        }
    }
    SortUniqueSelected();
}

void MultiSelectList::NotifySelectionChanged() {
    if (onSelect) {
        onSelect(selected);
    }
}

void MultiSelectList::HandleRowClick(const int idx, const GuiFrameInput& in) {
    const int n = VisibleRowCount();
    if (n <= 0 || idx < 0 || idx >= n) {
        return;
    }
    if (in.shiftDown) {
        const int a = std::clamp(anchorIndex, 0, n - 1);
        const int lo = std::min(a, idx);
        const int hi = std::max(a, idx);
        selected.Clear();
        for (int i = lo; i <= hi; ++i) {
            selected.PushBack(i);
        }
        SortUniqueSelected();
    } else if (in.ctrlDown) {
        anchorIndex = idx;
        if (IsSelected(idx)) {
            SetRowSelected(idx, false);
        } else {
            SetRowSelected(idx, true);
        }
    } else {
        anchorIndex = idx;
        selected.Clear();
        selected.PushBack(idx);
    }
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (Button* b = dynamic_cast<Button*>(children[j].Get())) {
            b->SetAccentSelected(IsSelected(static_cast<int>(j)));
        }
    }
    NotifySelectionChanged();
}

void MultiSelectList::Rebuild() {
    ClearChildren();
    for (std::size_t i = 0; i < items.GetSize(); ++i) {
        auto b = MakeUnique<Button>();
        Button* raw = b.Get();
        raw->SetLabel(items[i]);
        raw->SetFontSize(itemFontPx);
        raw->SetLabelBold(itemBold);
        raw->SetOpaqueSurface(opaqueRows);
        raw->ClearOnClickWithFrame();
        const int idx = static_cast<int>(i);
        raw->SetAccentSelected(IsSelected(idx));
        raw->SetOnClickWithFrame([this, idx](const GuiFrameInput& fin, GuiCanvasComponent&) {
            HandleRowClick(idx, fin);
        });
        AddChild(Spark::MoveTemp(b));
    }
}

void MultiSelectList::SetItems(Array<Utf8String> lines) {
    items = Spark::MoveTemp(lines);
    scrollY = 0.0F;
    selected.Clear();
    anchorIndex = 0;
    Rebuild();
}

void MultiSelectList::SetSelectedIndices(Array<int> indices) {
    selected = Spark::MoveTemp(indices);
    SortUniqueSelected();
    if (!items.IsEmpty()) {
        Rebuild();
    }
}

MultiSelectList::MultiSelectList() = default;

void MultiSelectList::SetVerticalScrollingEnabled(const bool enabled) noexcept {
    verticalScrollingEnabled = enabled;
    if (!enabled) {
        scrollY = 0.0F;
        draggingThumb = false;
    }
}

bool MultiSelectList::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

void MultiSelectList::UpdateScrollFromThumbTop(const float thumbTopY) {
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

void MultiSelectList::ScrollSelectionIntoView(const int idx) noexcept {
    if (!verticalScrollingEnabled || maxScroll <= 0.0F) {
        return;
    }
    const float rowTop = static_cast<float>(idx) * rowHeight;
    const float rowBottom = rowTop + rowHeight;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);
}

void MultiSelectList::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = children.GetSize();
    contentHeight = rowHeight * static_cast<float>(n);

    if (!verticalScrollingEnabled) {
        scrollY = 0.0F;
        maxScroll = 0.0F;
        bounds.height = std::max(r.height, contentHeight);
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
        for (std::size_t i = 0; i < n; ++i) {
            if (Button* b = dynamic_cast<Button*>(children[i].Get())) {
                b->SetAccentSelected(IsSelected(static_cast<int>(i)));
            }
            if (children[i]) {
                children[i]->Arrange({r.x, r.y + static_cast<float>(i) * rowHeight, r.width, rowHeight});
            }
        }
        return;
    }

    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - kTrackW) : r.width;
    const float viewportTop = r.y;

    for (std::size_t i = 0; i < n; ++i) {
        if (Button* b = dynamic_cast<Button*>(children[i].Get())) {
            b->SetAccentSelected(IsSelected(static_cast<int>(i)));
        }
        if (children[i]) {
            const float y = viewportTop - scrollY + static_cast<float>(i) * rowHeight;
            children[i]->Arrange({r.x, y, innerW, rowHeight});
        }
    }

    if (maxScroll <= 0.0F) {
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
    } else {
        trackRect = {r.x + innerW, r.y, kTrackW, r.height};
        if (contentHeight <= 0.0F) {
            thumbRect = {trackRect.x + 1.0F, trackRect.y + 1.0F, trackRect.width - 2.0F, trackRect.height - 2.0F};
        } else {
            constexpr float kMinThumb = 22.0F;
            const float thumbH =
                    std::max(kMinThumb, r.height * (r.height / std::max(contentHeight, r.height)));
            const float travel = std::max(0.0F, trackRect.height - thumbH);
            const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
            thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
        }
    }
}

void MultiSelectList::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    if (maxScroll <= 0.0F) {
        Widget::Paint(ctx);
        return;
    }

    const float innerW = std::max(0.0F, bounds.width - kTrackW);
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    const float clipW = std::max(1.0F, clipR - clipX);
    const float clipH = std::max(1.0F, clipB - clipY);

    ctx.PushClipRect(clipX, clipY, clipW, clipH);
    PaintChildren(ctx);
    ctx.PopClipRect();

    const GuiTheme& th = ctx.GetTheme();
    ctx.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, th.insetTrackRgb, 0.92F);
    ctx.FillRectGradientVertical(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, th.thumbGradientTop,
            th.thumbGradientBottom, 0.95F);
    ctx.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, th.borderRgb, 0.5F);
}

Widget* MultiSelectList::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    return Widget::FindDeepestHover(x, y);
}

void MultiSelectList::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
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
    }
}

void MultiSelectList::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
}

void MultiSelectList::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    draggingThumb = false;
}

void MultiSelectList::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    static constexpr float kPixelsPerWheelUnit = 48.0F;
    scrollY = std::clamp(scrollY - deltaY * kPixelsPerWheelUnit, 0.0F, maxScroll);
}

void MultiSelectList::ProcessKeyInput(IInput& input) {
    if (!enabled || items.IsEmpty()) {
        return;
    }
    const int n = VisibleRowCount();
    if (n <= 0) {
        return;
    }
    int cur = anchorIndex;
    if (!selected.IsEmpty()) {
        cur = selected.GetLast();
    }
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
    } else {
        return;
    }
    anchorIndex = next;
    if (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT)) {
        const int lo = std::min(anchorIndex, next);
        const int hi = std::max(anchorIndex, next);
        selected.Clear();
        for (int i = lo; i <= hi; ++i) {
            selected.PushBack(i);
        }
        SortUniqueSelected();
    } else if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL)) {
        SetRowSelected(next, !IsSelected(next));
    } else {
        anchorIndex = next;
        selected.Clear();
        selected.PushBack(next);
    }
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (Button* b = dynamic_cast<Button*>(children[j].Get())) {
            b->SetAccentSelected(IsSelected(static_cast<int>(j)));
        }
    }
    ScrollSelectionIntoView(next);
    NotifySelectionChanged();
}

}  // namespace Spark::Gui
