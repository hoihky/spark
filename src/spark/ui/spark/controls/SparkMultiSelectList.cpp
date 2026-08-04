#include "spark/ui/spark/controls/SparkMultiSelectList.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Ui {

namespace {

float Clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
}

void MultiSelectRowClickThunk(void* userData, const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (userData != nullptr) {
        auto* binding = static_cast<SparkMultiSelectList::RowBinding*>(userData);
        if (binding->list != nullptr) {
            binding->list->HandleRowClick(binding->index, input);
        }
    }
}

}  // namespace

SparkMultiSelectList::SparkMultiSelectList(const MultiSelectListDesc& desc)
    : UiElementBase(desc.id)
    , rowHeight(desc.rowHeight)
    , itemFontPx(desc.itemFontSize)
    , itemBold(desc.itemBold)
    , opaqueRows(desc.opaqueRows)
    , verticalScrollingEnabled(desc.verticalScrollingEnabled) {}

float SparkMultiSelectList::ScaledRowHeight() const noexcept {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    return metrics.Scaled(rowHeight > 0.0F ? rowHeight : metrics.listRowHeight);
}

void SparkMultiSelectList::SetItems(Array<Utf8String> itemsIn) {
    items = MoveTemp(itemsIn);
    scrollY = 0.0F;
    selected.Clear();
    anchorIndex = 0;
    RebuildRows();
}

void SparkMultiSelectList::SetSelectedIndices(Array<int> indices) {
    selected = MoveTemp(indices);
    SortUniqueSelected();
    if (!items.IsEmpty()) {
        RebuildRows();
    }
}

void SparkMultiSelectList::SetScrollY(const float y) {
    scrollY = y;
}

void SparkMultiSelectList::SortUniqueSelected() {
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

bool SparkMultiSelectList::IsSelected(const int idx) const noexcept {
    for (std::size_t i = 0; i < selected.GetSize(); ++i) {
        if (selected[i] == idx) {
            return true;
        }
    }
    return false;
}

void SparkMultiSelectList::SetRowSelected(const int idx, const bool sel) {
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

void SparkMultiSelectList::NotifySelectionChanged() {
    onSelect.Invoke();
}

void SparkMultiSelectList::HandleRowClick(const int idx, const UiFrameInput& input) {
    const int n = VisibleRowCount();
    if (n <= 0 || idx < 0 || idx >= n) {
        return;
    }
    if (input.shiftDown) {
        const int a = std::clamp(anchorIndex, 0, n - 1);
        const int lo = std::min(a, idx);
        const int hi = std::max(a, idx);
        selected.Clear();
        for (int i = lo; i <= hi; ++i) {
            selected.PushBack(i);
        }
        SortUniqueSelected();
    } else if (input.ctrlDown) {
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
        if (auto* button = dynamic_cast<SparkButton*>(children[j].Get())) {
            button->SetAccentSelected(IsSelected(static_cast<int>(j)));
        }
    }
    NotifySelectionChanged();
}

void SparkMultiSelectList::RebuildRows() {
    rowBindings.Clear();
    children.Clear();
    rowBindings.Reserve(items.GetSize());
    for (std::size_t i = 0; i < items.GetSize(); ++i) {
        ButtonDesc desc{};
        desc.id = Utf8String("ms-row");
        desc.label = items[i];
        auto button = MakeUnique<SparkButton>(desc);
        if (itemFontPx > 0.0F) {
            button->SetFontSize(itemFontPx);
        }
        button->SetLabelBold(itemBold);
        button->SetOpaqueSurface(opaqueRows);
        button->SetAccentSelected(IsSelected(static_cast<int>(i)));
        RowBinding binding{};
        binding.list = this;
        binding.index = static_cast<int>(i);
        rowBindings.PushBack(binding);
        UiFrameVoidCallback click{};
        click.fn = &MultiSelectRowClickThunk;
        click.userData = &rowBindings[rowBindings.GetSize() - 1U];
        button->SetOnClickWithFrame(click);
        AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(button.Release())));
    }
}

void SparkMultiSelectList::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

bool SparkMultiSelectList::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

void SparkMultiSelectList::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = std::max(0.0F, trackRect.height - thumbRect.height);
    if (travel <= 1.0e-4F) {
        return;
    }
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = Clampf(t, 0.0F, 1.0F) * maxScroll;
}

void SparkMultiSelectList::ScrollSelectionIntoView(const int idx) noexcept {
    if (!verticalScrollingEnabled || maxScroll <= 0.0F) {
        return;
    }
    const float scaledRowH = ScaledRowHeight();
    const float rowTop = static_cast<float>(idx) * scaledRowH;
    const float rowBottom = rowTop + scaledRowH;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = Clampf(scrollY, 0.0F, maxScroll);
}

void SparkMultiSelectList::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const float scaledRowH = ScaledRowHeight();
    outDesired.width = Clampf(GetActiveUiLayoutMetrics().Scaled(200.0F), constraints.minWidth, constraints.maxWidth);
    outDesired.height = Clampf(scaledRowH * static_cast<float>((std::max)(items.GetSize(), std::size_t{1})),
            constraints.minHeight,
            constraints.maxHeight);
}

void SparkMultiSelectList::Arrange(const Rect& r) {
    arrangeRect = r;
    bounds = r;
    if (!visible || r.width < 0.5F || r.height < 0.5F) {
        maxScroll = 0.0F;
        scrollY = 0.0F;
        trackRect = {};
        thumbRect = {};
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                children[i]->Arrange(Rect{});
            }
        }
        return;
    }

    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float scaledRowH = ScaledRowHeight();
    const float trackW = metrics.Scaled(kTrackWidth);
    const std::size_t n = children.GetSize();
    contentHeight = scaledRowH * static_cast<float>(n);

    if (!verticalScrollingEnabled) {
        scrollY = 0.0F;
        maxScroll = 0.0F;
        bounds.height = std::max(r.height, contentHeight);
        trackRect = {};
        thumbRect = {};
        for (std::size_t i = 0; i < n; ++i) {
            if (auto* button = dynamic_cast<SparkButton*>(children[i].Get())) {
                button->SetAccentSelected(IsSelected(static_cast<int>(i)));
            }
            if (children[i] != nullptr) {
                children[i]->Arrange({r.x, r.y + static_cast<float>(i) * scaledRowH, r.width, scaledRowH});
            }
        }
        return;
    }

    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = Clampf(scrollY, 0.0F, maxScroll);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - trackW) : r.width;
    const float viewportTop = r.y;

    for (std::size_t i = 0; i < n; ++i) {
        if (auto* button = dynamic_cast<SparkButton*>(children[i].Get())) {
            button->SetAccentSelected(IsSelected(static_cast<int>(i)));
        }
        if (children[i] != nullptr) {
            const float y = viewportTop - scrollY + static_cast<float>(i) * scaledRowH;
            children[i]->Arrange({r.x, y, innerW, scaledRowH});
        }
    }

    if (maxScroll <= 0.0F) {
        trackRect = {};
        thumbRect = {};
    } else {
        trackRect = {r.x + innerW, r.y, trackW, r.height};
        if (contentHeight <= 0.0F) {
            thumbRect = {trackRect.x + 1.0F, trackRect.y + 1.0F, trackRect.width - 2.0F, trackRect.height - 2.0F};
        } else {
            const float kMinThumb = metrics.Scaled(22.0F);
            const float thumbH =
                    std::max(kMinThumb, r.height * (r.height / std::max(contentHeight, r.height)));
            const float travel = std::max(0.0F, trackRect.height - thumbH);
            const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
            thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
        }
    }
}

void SparkMultiSelectList::Paint(IUiRenderer& renderer) {
    if (!visible || bounds.height < 0.5F || bounds.width < 0.5F) {
        return;
    }
    if (maxScroll <= 0.0F) {
        UiElementBase::Paint(renderer);
        return;
    }

    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const UiTheme& theme = renderer.GetTheme();
    const float trackW = metrics.Scaled(kTrackWidth);
    const float innerW = std::max(0.0F, bounds.width - trackW);
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    const float clipW = std::max(1.0F, clipR - clipX);
    const float clipH = std::max(1.0F, clipB - clipY);
    const float overflowTop = std::min(scrollY, clipY);
    const float fillY = clipY - overflowTop;
    const float fillH = clipH + overflowTop;

    renderer.FillRectGradientVertical(
            clipX, fillY, clipW, fillH, theme.scrollViewportTop, theme.scrollViewportBottom, theme.scrollViewportAlpha);

    renderer.PushClip(Rect{clipX, fillY, clipW, fillH});
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr || !children[i]->IsVisible()) {
            continue;
        }
        const Rect rowBounds = children[i]->GetBounds();
        const float overflowTopRow = std::min(scrollY, bounds.y);
        const float viewTop = bounds.y - overflowTopRow;
        const float viewBottom = bounds.y + bounds.height;
        if (rowBounds.y + rowBounds.height <= viewTop || rowBounds.y >= viewBottom) {
            continue;
        }
        children[i]->Paint(renderer);
    }
    renderer.PopClip();

    renderer.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, theme.insetTrackRgb, 0.92F);
    renderer.FillRoundRectGradientVertical(
            thumbRect.x,
            thumbRect.y,
            thumbRect.width,
            thumbRect.height,
            metrics.Scaled(3.0F),
            theme.thumbGradientTop,
            theme.thumbGradientBottom,
            0.95F);
    renderer.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, theme.borderRgb, 0.5F);
}

IUiElement* SparkMultiSelectList::HitTest(const float x, const float y) {
    if (!WantsHitTest() || bounds.height < 0.5F || bounds.width < 0.5F) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    return UiElementBase::HitTest(x, y);
}

void SparkMultiSelectList::OnPointerDown(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled) {
        return;
    }
    if (HitTrack(input.mouseX, input.mouseY)) {
        draggingThumb = true;
        grabOffsetY = thumbRect.Contains(input.mouseX, input.mouseY) ? input.mouseY - thumbRect.y : thumbRect.height * 0.5F;
        UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
        SyncScrollLayout();
    }
}

void SparkMultiSelectList::OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void SparkMultiSelectList::OnPointerUp(const UiFrameInput& /*input*/, UiCanvasComponent& /*canvas*/) {
    draggingThumb = false;
}

void SparkMultiSelectList::OnScroll(const float /*deltaX*/, const float deltaY) {
    ApplyScrollWheelDelta(deltaY);
}

void SparkMultiSelectList::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    const float scaledRowH = ScaledRowHeight();
    const int steps = static_cast<int>(std::round(deltaY));
    if (steps == 0) {
        return;
    }
    scrollY = Clampf(scrollY - static_cast<float>(steps) * scaledRowH, 0.0F, maxScroll);
    SyncScrollLayout();
}

void SparkMultiSelectList::ProcessKeyInput(IInput& input) {
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
        if (auto* button = dynamic_cast<SparkButton*>(children[j].Get())) {
            button->SetAccentSelected(IsSelected(static_cast<int>(j)));
        }
    }
    ScrollSelectionIntoView(next);
    NotifySelectionChanged();
}

}  // namespace Spark::Ui
