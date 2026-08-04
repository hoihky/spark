#include "spark/ui/spark/controls/SparkScrollPanel.hpp"

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"

namespace Spark::Ui {

namespace {

float Clampf(const float v, const float lo, const float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

float MaxVisibleSubtreeBottom(const IUiElement* element) {
    if (element == nullptr || !element->IsVisible()) {
        return -1.0e30F;
    }
    const Rect b = element->GetBounds();
    float maxBottom = b.y + b.height;
    const Array<UniquePtr<IUiElement>>& children = element->GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        const float childBottom = MaxVisibleSubtreeBottom(children[i].Get());
        if (childBottom > maxBottom) {
            maxBottom = childBottom;
        }
    }
    return maxBottom;
}

}  // namespace

SparkScrollPanel::SparkScrollPanel(const ScrollPanelDesc& desc)
    : UiElementBase(desc.id), designHeight(desc.height), rowHeight(desc.rowHeight), vGap(desc.verticalGap) {}

void SparkScrollPanel::SetScrollY(const float y) noexcept {
    scrollY = y;
}

void SparkScrollPanel::ScrollToTop() noexcept {
    scrollY = 0.0F;
    SyncScrollLayout();
}

void SparkScrollPanel::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    float w = metrics.Scaled(200.0F);
    if (w < constraints.minWidth) {
        w = constraints.minWidth;
    }
    if (w > constraints.maxWidth) {
        w = constraints.maxWidth;
    }
    float h = designHeight > 0.0F ? metrics.Scaled(designHeight) : metrics.Scaled(240.0F);
    if (h < constraints.minHeight) {
        h = constraints.minHeight;
    }
    if (h > constraints.maxHeight) {
        h = constraints.maxHeight;
    }
    outDesired.width = w;
    outDesired.height = h;
}

void SparkScrollPanel::Arrange(const Rect& finalBounds) {
    arrangeRect = finalBounds;
    bounds = finalBounds;
    if (!visible || finalBounds.height <= 0.0F || finalBounds.width <= 0.0F) {
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                children[i]->Arrange(Rect{finalBounds.x, finalBounds.y, finalBounds.width, 0.0F});
            }
        }
        contentHeight = 0.0F;
        maxScroll = 0.0F;
        trackRect = {};
        thumbRect = {};
        return;
    }

    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float gap = metrics.Scaled(vGap);
    const float uniformRow = rowHeight > 0.0F ? metrics.Scaled(rowHeight) : metrics.FormRowHeight();
    const float innerW = finalBounds.width - kTrackWidth > 0.0F ? finalBounds.width - kTrackWidth : 0.0F;
    const Rect inner{finalBounds.x, finalBounds.y, innerW, finalBounds.height};
    const float viewportTop = inner.y;
    const std::size_t childCount = children.GetSize();

    Array<float> effectiveHeights;
    effectiveHeights.Resize(childCount);
    float measureY = 0.0F;
    for (std::size_t i = 0; i < childCount; ++i) {
        if (children[i] == nullptr || !children[i]->IsVisible()) {
            effectiveHeights[i] = 0.0F;
            continue;
        }
        children[i]->Arrange(Rect{inner.x, inner.y + measureY, innerW, uniformRow});
        const float top = children[i]->GetBounds().y;
        const float deep = MaxVisibleSubtreeBottom(children[i].Get());
        effectiveHeights[i] = uniformRow;
        const float measured = deep - top;
        if (measured > effectiveHeights[i]) {
            effectiveHeights[i] = measured;
        }
        if (effectiveHeights[i] < 4.0F) {
            effectiveHeights[i] = 4.0F;
        }
        measureY += effectiveHeights[i];
        if (i + 1U < childCount) {
            measureY += gap;
        }
    }

    contentHeight = 0.0F;
    for (std::size_t i = 0; i < childCount; ++i) {
        if (effectiveHeights[i] <= 0.0F) {
            continue;
        }
        contentHeight += effectiveHeights[i];
        if (i + 1U < childCount) {
            contentHeight += gap;
        }
    }

    maxScroll = contentHeight - inner.height > 0.0F ? contentHeight - inner.height : 0.0F;
    scrollY = Clampf(scrollY, 0.0F, maxScroll);

    float y = 0.0F;
    for (std::size_t i = 0; i < childCount; ++i) {
        if (children[i] == nullptr || !children[i]->IsVisible() || effectiveHeights[i] <= 0.0F) {
            continue;
        }
        children[i]->Arrange(Rect{inner.x, viewportTop - scrollY + y, innerW, effectiveHeights[i]});
        y += effectiveHeights[i];
        if (i + 1U < childCount) {
            y += gap;
        }
    }

    trackRect = Rect{finalBounds.x + innerW, finalBounds.y, kTrackWidth, finalBounds.height};
    if (maxScroll <= 0.0F || contentHeight <= 0.0F) {
        thumbRect = Rect{trackRect.x + 1.0F, trackRect.y + 1.0F, trackRect.width - 2.0F, trackRect.height - 2.0F};
    } else {
        const float minThumb = 22.0F;
        const float thumbH = inner.height * (inner.height / (contentHeight > 0.0F ? contentHeight : inner.height));
        const float thumbHeight = thumbH > minThumb ? thumbH : minThumb;
        const float travel = trackRect.height - thumbHeight > 0.0F ? trackRect.height - thumbHeight : 0.0F;
        const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
        thumbRect = Rect{trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbHeight};
    }
}

void SparkScrollPanel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    const UiTheme& theme = renderer.GetTheme();
    const float innerW = bounds.width - kTrackWidth > 0.0F ? bounds.width - kTrackWidth : 0.0F;
    const Rect clipRect{bounds.x, bounds.y, innerW, bounds.height};
    renderer.PushClip(clipRect);
    renderer.FillRectGradientVertical(
            bounds.x,
            bounds.y,
            innerW,
            bounds.height,
            theme.scrollViewportTop,
            theme.scrollViewportBottom,
            theme.scrollViewportAlpha);
    renderer.StrokeRect(bounds.x, bounds.y, innerW, bounds.height, 1.0F, theme.borderRgb, 0.62F);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
    renderer.PopClip();

    renderer.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, theme.insetTrackRgb, 0.92F);
    renderer.FillRectGradientVertical(
            thumbRect.x,
            thumbRect.y,
            thumbRect.width,
            thumbRect.height,
            theme.thumbGradientTop,
            theme.thumbGradientBottom,
            0.95F);
    renderer.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, theme.borderRgb, 0.5F);
}

IUiElement* SparkScrollPanel::HitTest(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (HitTrack(x, y)) {
        return this;
    }
    const float innerW = bounds.width - kTrackWidth > 0.0F ? bounds.width - kTrackWidth : 0.0F;
    const Rect content{bounds.x, bounds.y, innerW, bounds.height};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    for (std::size_t ci = children.GetSize(); ci > 0; --ci) {
        IUiElement* child = children[ci - 1U].Get();
        if (child == nullptr || !child->IsVisible()) {
            continue;
        }
        const Rect childBounds = child->GetBounds();
        if (childBounds.y + childBounds.height <= bounds.y || childBounds.y >= bounds.y + bounds.height) {
            continue;
        }
        if (IUiElement* hit = child->HitTest(x, y)) {
            return hit;
        }
    }
    if (hitTest && content.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

const IUiElement* SparkScrollPanel::HitTest(const float x, const float y) const {
    return const_cast<SparkScrollPanel*>(this)->HitTest(x, y);
}

void SparkScrollPanel::OnPointerDown(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled) {
        return;
    }
    if (!HitTrack(input.mouseX, input.mouseY)) {
        return;
    }
    draggingThumb = true;
    if (thumbRect.Contains(input.mouseX, input.mouseY)) {
        grabOffsetY = input.mouseY - thumbRect.y;
    } else {
        grabOffsetY = thumbRect.height * 0.5F;
        UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
    }
    SyncScrollLayout();
}

void SparkScrollPanel::OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void SparkScrollPanel::OnPointerUp(const UiFrameInput& /*input*/, UiCanvasComponent& /*canvas*/) {
    draggingThumb = false;
}

void SparkScrollPanel::OnScroll(const float /*deltaX*/, const float deltaY) {
    ApplyScrollWheelDelta(deltaY);
}

bool SparkScrollPanel::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

void SparkScrollPanel::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = trackRect.height - thumbRect.height > 0.0F ? trackRect.height - thumbRect.height : 0.0F;
    if (travel <= 1.0e-4F) {
        return;
    }
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = Clampf(t, 0.0F, 1.0F) * maxScroll;
}

void SparkScrollPanel::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

void SparkScrollPanel::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    constexpr float kPixelsPerWheelUnit = 48.0F;
    scrollY = Clampf(scrollY - deltaY * kPixelsPerWheelUnit, 0.0F, maxScroll);
    SyncScrollLayout();
}

}  // namespace Spark::Ui
