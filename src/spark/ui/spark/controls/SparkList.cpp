#include "spark/ui/spark/controls/SparkList.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>

#include <algorithm>

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Ui {

namespace {

void RowClickThunk(void* userData) {
    if (userData != nullptr) {
        auto* binding = static_cast<SparkList::RowBinding*>(userData);
        if (binding->list != nullptr) {
            binding->list->HandleRowClick(binding->index);
        }
    }
}

float Clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
}

}  // namespace

SparkList::SparkList(const ListDesc& desc)
    : UiElementBase(desc.id)
    , rowHeight(desc.rowHeight > 0.0F ? desc.rowHeight : 30.0F)
    , itemFontPx(desc.itemFontSize)
    , itemBold(desc.itemBold)
    , opaqueRows(desc.opaqueRows)
    , verticalScrollingEnabled(desc.verticalScrollingEnabled) {}

void SparkList::SetItems(Array<Utf8String> itemsIn) {
    items = MoveTemp(itemsIn);
    scrollY = 0.0F;
    RebuildRows();
}

void SparkList::SetSelectedIndex(const int index) {
    if (items.IsEmpty() || children.IsEmpty()) {
        selectedIndex = -1;
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    selectedIndex = std::clamp(index, 0, n - 1);
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (auto* button = dynamic_cast<SparkButton*>(children[j].Get())) {
            button->SetAccentSelected(static_cast<int>(j) == selectedIndex);
        }
    }
}

void SparkList::SetScrollY(const float y) {
    scrollY = y;
}

void SparkList::ScrollToTop() noexcept {
    scrollY = 0.0F;
    SyncScrollLayout();
}

void SparkList::HandleRowClick(const int index) {
    SetSelectedIndex(index);
    onSelect.Invoke(index);
}

void SparkList::RebuildRows() {
    rowBindings.Clear();
    children.Clear();
    rowBindings.Reserve(items.GetSize());
    for (std::size_t i = 0; i < items.GetSize(); ++i) {
        ButtonDesc desc{};
        char rowId[32];
        std::snprintf(rowId, sizeof(rowId), "row.%zu", i);
        desc.id = Utf8String(rowId);
        desc.label = items[i];
        auto button = MakeUnique<SparkButton>(desc);
        if (itemFontPx > 0.0F) {
            button->SetFontSize(itemFontPx);
        }
        button->SetLabelBold(itemBold);
        button->SetOpaqueSurface(opaqueRows);
        RowBinding binding{};
        binding.list = this;
        binding.index = static_cast<int>(i);
        rowBindings.PushBack(binding);
        UiVoidCallback click{};
        click.fn = &RowClickThunk;
        click.userData = &rowBindings[rowBindings.GetSize() - 1U];
        button->SetOnClick(click);
        AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(button.Release())));
    }
    if (selectedIndex >= static_cast<int>(items.GetSize())) {
        selectedIndex = items.IsEmpty() ? -1 : 0;
    }
    SetSelectedIndex(selectedIndex);
}

void SparkList::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

bool SparkList::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

bool SparkList::RowIntersectsViewport(const Rect& rowBounds) const noexcept {
    if (maxScroll <= 0.0F) {
        return true;
    }
    const float viewTop = bounds.y;
    const float viewBottom = bounds.y + bounds.height;
    return rowBounds.y + rowBounds.height > viewTop && rowBounds.y < viewBottom;
}

void SparkList::UpdateScrollFromThumbTop(const float thumbTopY) {
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

void SparkList::SnapScrollToRowGrid(const float scaledRowH) noexcept {
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
    scrollY = Clampf(scrollY, 0.0F, maxScroll);
}

void SparkList::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float scaledRowH = metrics.Scaled(rowHeight);
    outDesired.width = Clampf(metrics.Scaled(200.0F), constraints.minWidth, constraints.maxWidth);
    outDesired.height = Clampf(scaledRowH * static_cast<float>((std::max)(items.GetSize(), std::size_t{1})),
            constraints.minHeight,
            constraints.maxHeight);
}

void SparkList::Arrange(const Rect& r) {
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
    const float scaledRowH = metrics.Scaled(rowHeight);
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
                button->SetAccentSelected(static_cast<int>(i) == selectedIndex);
            }
            if (children[i] != nullptr) {
                children[i]->Arrange({r.x, r.y + static_cast<float>(i) * scaledRowH, r.width, scaledRowH});
            }
        }
        return;
    }

    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = Clampf(scrollY, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - trackW) : r.width;
    const float viewportTop = r.y;

    for (std::size_t i = 0; i < n; ++i) {
        if (auto* button = dynamic_cast<SparkButton*>(children[i].Get())) {
            button->SetAccentSelected(static_cast<int>(i) == selectedIndex);
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

void SparkList::Paint(IUiRenderer& renderer) {
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
        if (!RowIntersectsViewport(children[i]->GetBounds())) {
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

IUiElement* SparkList::HitTest(const float x, const float y) {
    if (!WantsHitTest() || bounds.height < 0.5F || bounds.width < 0.5F) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float trackW = metrics.Scaled(kTrackWidth);
    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, bounds.width - trackW) : bounds.width;
    const float overflowTop = std::min(scrollY, bounds.y);
    const Rect content{bounds.x, bounds.y - overflowTop, innerW, bounds.height + overflowTop};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    for (std::size_t i = children.GetSize(); i > 0; --i) {
        IUiElement* child = children[i - 1U].Get();
        if (child == nullptr || !child->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(child->GetBounds())) {
            continue;
        }
        if (IUiElement* hit = child->HitTest(x, y)) {
            return hit;
        }
    }
    return this;
}

void SparkList::OnPointerDown(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
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

void SparkList::OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void SparkList::OnPointerUp(const UiFrameInput& /*input*/, UiCanvasComponent& /*canvas*/) {
    if (draggingThumb) {
        const float scaledRowH = GetActiveUiLayoutMetrics().Scaled(rowHeight);
        SnapScrollToRowGrid(scaledRowH);
        SyncScrollLayout();
    }
    draggingThumb = false;
}

void SparkList::OnScroll(const float /*deltaX*/, const float deltaY) {
    ApplyScrollWheelDelta(deltaY);
}

void SparkList::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    const float scaledRowH = GetActiveUiLayoutMetrics().Scaled(rowHeight);
    const int steps = static_cast<int>(std::round(deltaY));
    if (steps == 0) {
        return;
    }
    scrollY = Clampf(scrollY - static_cast<float>(steps) * scaledRowH, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);
    SyncScrollLayout();
}

void SparkList::ScrollSelectedIndexIntoView() noexcept {
    if (!verticalScrollingEnabled || maxScroll <= 0.0F || selectedIndex < 0) {
        return;
    }
    const float scaledRowH = GetActiveUiLayoutMetrics().Scaled(rowHeight);
    const float rowTop = static_cast<float>(selectedIndex) * scaledRowH;
    const float rowBottom = rowTop + scaledRowH;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = Clampf(scrollY, 0.0F, maxScroll);
    SnapScrollToRowGrid(scaledRowH);
    SyncScrollLayout();
}

void SparkList::ProcessKeyInput(IInput& input) {
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
        if (selectedIndex >= 0) {
            onSelect.Invoke(selectedIndex);
        }
        return;
    } else {
        return;
    }
    SetSelectedIndex(next);
    onSelect.Invoke(next);
    ScrollSelectedIndexIntoView();
}

}  // namespace Spark::Ui
