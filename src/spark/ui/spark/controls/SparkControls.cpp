#include "spark/ui/spark/controls/SparkControls.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/controls/SparkList.hpp"
#include "spark/ui/spark/controls/SparkScrollPanel.hpp"

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

float ClampWidth(const UiMeasureConstraints& constraints, const float desired) {
    return Clampf(desired, constraints.minWidth, constraints.maxWidth);
}

float ClampHeight(const UiMeasureConstraints& constraints, const float desired) {
    return Clampf(desired, constraints.minHeight, constraints.maxHeight);
}

[[nodiscard]] bool ChildWantsVerticalFlex(const IUiElement* element) noexcept {
    if (element == nullptr) {
        return false;
    }
    if (dynamic_cast<const SparkScrollPanel*>(element) != nullptr) {
        return true;
    }
    if (const auto* list = dynamic_cast<const SparkList*>(element)) {
        return list->WantsScrollInput();
    }
    return false;
}

[[nodiscard]] std::size_t FindLastFlexChildIndex(const Array<UniquePtr<IUiElement>>& children) noexcept {
    for (std::size_t i = children.GetSize(); i > 0; --i) {
        if (ChildWantsVerticalFlex(children[i - 1U].Get())) {
            return i - 1U;
        }
    }
    return children.GetSize();
}

void StackChildrenVertically(
        const Array<UniquePtr<IUiElement>>& children,
        const Rect& inner,
        const float rowGap) {
    float y = inner.y;
    float remainingH = inner.height;
    const std::size_t count = children.GetSize();
    const std::size_t lastFlexIndex = FindLastFlexChildIndex(children);
    for (std::size_t i = 0; i < count; ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        if (remainingH <= 0.0F) {
            children[i]->Arrange(Rect{inner.x, y, inner.width, 0.0F});
            continue;
        }
        const bool flex = ChildWantsVerticalFlex(children[i].Get());
        const bool expandFlex = flex && i == lastFlexIndex;
        UiMeasureConstraints rowConstraints{};
        rowConstraints.minWidth = inner.width;
        rowConstraints.maxWidth = inner.width;
        rowConstraints.minHeight = 0.0F;
        rowConstraints.maxHeight = remainingH;
        UiSize desired{};
        children[i]->Measure(rowConstraints, desired);
        float rowH = desired.height > 0.0F ? desired.height : 1.0F;
        if (expandFlex) {
            rowH = remainingH;
        }
        rowH = std::min(rowH, remainingH);
        children[i]->Arrange(Rect{inner.x, y, inner.width, rowH});
        remainingH -= rowH + rowGap;
        y += rowH + rowGap;
    }
}

UiSize MeasureStackedChildren(
        const Array<UniquePtr<IUiElement>>& children,
        const UiMeasureConstraints& constraints,
        const float rowGap,
        const float minWidth) {
    UiSize out{};
    out.width = minWidth;
    float remainingH = constraints.maxHeight > 0.0F ? constraints.maxHeight : 1.0e9F;
    const std::size_t count = children.GetSize();
    const std::size_t lastFlexIndex = FindLastFlexChildIndex(children);
    for (std::size_t i = 0; i < count; ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        const bool flex = ChildWantsVerticalFlex(children[i].Get());
        const bool expandFlex = flex && i == lastFlexIndex;
        UiMeasureConstraints rowConstraints = constraints;
        rowConstraints.maxHeight = remainingH;
        UiSize childDesired{};
        children[i]->Measure(rowConstraints, childDesired);
        float rowH = childDesired.height > 0.0F ? childDesired.height : 1.0F;
        if (expandFlex && remainingH > 0.0F) {
            rowH = remainingH;
        } else {
            rowH = std::min(rowH, remainingH);
        }
        if (childDesired.width > out.width) {
            out.width = childDesired.width;
        }
        out.height += rowH;
        if (i + 1U < count) {
            out.height += rowGap;
        }
        remainingH -= rowH + rowGap;
        if (remainingH <= 0.0F) {
            break;
        }
    }
    return out;
}

[[nodiscard]] Rect PanelInnerContentRect(const Rect& panelBounds, const Utf8String& title, const UiLayoutMetrics& metrics) {
    const float pad = metrics.Padding();
    const float titleH = title.IsEmpty() ? 0.0F : metrics.FontLabel() + metrics.Scaled(8.0F);
    Rect inner = panelBounds.Inset(pad);
    inner.y += titleH;
    inner.height -= titleH;
    return inner;
}

}  // namespace

SparkButton::SparkButton(const ButtonDesc& desc) : UiElementBase(desc.id), label(desc.label) {
    SetEnabled(desc.enabled);
}

void SparkButton::SetLabel(Utf8String labelIn) {
    label = MoveTemp(labelIn);
}

void SparkButton::OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) {
    clickedThisFrame = false;
    if (!IsEnabled() || !GetBounds().Contains(input.mouseX, input.mouseY)) {
        return;
    }
    clickedThisFrame = true;
    if (onClickWithFrame.IsBound()) {
        onClickWithFrame.Invoke(input, canvas);
    } else {
        onClick.Invoke();
    }
}

void SparkButton::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(120.0F));
    outDesired.height = ClampHeight(constraints, metrics.FormRowHeight());
}

void SparkButton::DoPaint(IUiRenderer& renderer) {
    clickedThisFrame = false;
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect b = GetBounds();
    const float drawFont = fontPx > 0.0F ? metrics.Scaled(fontPx) : metrics.FontControl();
    const float padX = metrics.Scaled(10.0F);
    const float shadowOff = metrics.Scaled(2.5F);
    const float shadowBlur = metrics.Scaled(3.0F);
    const float cornerR = metrics.Scaled(theme.controlCornerRadius);

    Vector3 top = theme.controlIdleTop;
    Vector3 bot = theme.controlIdleBottom;
    Vector3 textRgb = theme.labelPrimary;
    if (accentSelected) {
        top = theme.controlActiveTop;
        bot = theme.controlActiveBottom;
    } else if (renderer.IsHot(this)) {
        top = theme.controlHotTop;
        bot = theme.controlHotBottom;
    }
    if (renderer.IsActive(this)) {
        top = theme.controlActiveTop;
        bot = theme.controlActiveBottom;
    }

    if (!opaqueSurface) {
        renderer.FillDropShadow(b.x, b.y, b.width, b.height, shadowOff, shadowBlur, theme.shadowRgb, 0.72F);
    }
    renderer.FillRoundRectGradientVertical(
            b.x, b.y, b.width, b.height, cornerR, top, bot, opaqueSurface ? 1.0F : theme.controlFillAlpha);
    renderer.StrokeRoundRect(
            b.x, b.y, b.width, b.height, cornerR, 1.0F, theme.borderRgb, theme.controlStrokeAlpha);

    if (!label.IsEmpty()) {
        const float textY = b.y + (b.height - drawFont) * 0.5F;
        const float textX = std::floor(b.x + padX) + 0.5F;
        const Utf8String drawn = renderer.EllipsizeUtf8(label, drawFont, b.width - padX * 2.0F);
        renderer.DrawText(textX, textY, b.width - padX * 2.0F, drawn, textRgb, 1.0F, drawFont, labelBold);
    }
}

SparkPanel::SparkPanel(const PanelDesc& desc)
    : UiElementBase(desc.id)
    , title(desc.title)
    , open(desc.open)
    , designWidth(desc.width)
    , designHeight(desc.height)
    , anchorRight(desc.anchorRight)
    , edgeMargin(desc.edgeMargin)
    , centerInParent(desc.centerInParent) {}

void SparkPanel::SetTitle(Utf8String titleIn) {
    title = MoveTemp(titleIn);
}

void SparkPanel::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float pad = metrics.Padding();
    const float titleH = title.IsEmpty() ? 0.0F : metrics.FontLabel() + metrics.Scaled(8.0F);
    UiSize content = MeasureStackedChildren(children, constraints, metrics.ControlGap(), metrics.Scaled(200.0F));
    float w = designWidth > 0.0F ? metrics.Scaled(designWidth) : content.width + pad * 2.0F;
    float h = designHeight > 0.0F ? metrics.Scaled(designHeight) : content.height + pad * 2.0F + titleH;
    outDesired.width = ClampWidth(constraints, w);
    outDesired.height = ClampHeight(constraints, h);
}

void SparkPanel::Arrange(const Rect& finalBounds) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float margin = metrics.Scaled(edgeMargin);
    float w = designWidth > 0.0F ? metrics.Scaled(designWidth) : finalBounds.width;
    float h = designHeight > 0.0F ? metrics.Scaled(designHeight) : finalBounds.height - margin * 2.0F;
    const float maxH = std::max(0.0F, finalBounds.height - margin * 2.0F);
    const float maxW = std::max(0.0F, finalBounds.width - margin * 2.0F);
    if (designWidth > 0.0F) {
        w = std::min(w, maxW);
    }
    if (designHeight > 0.0F) {
        h = std::min(h, maxH);
    }
    if (anchorRight) {
        bounds = Rect{
                finalBounds.x + finalBounds.width - w - margin,
                finalBounds.y + margin,
                w,
                h};
    } else if (centerInParent) {
        bounds = Rect{
                finalBounds.x + (finalBounds.width - w) * 0.5F,
                finalBounds.y + (finalBounds.height - h) * 0.5F,
                w,
                h};
    } else {
        bounds = Rect{finalBounds.x, finalBounds.y, w, h};
    }
    DoArrangeChildren();
}

void SparkPanel::DoArrangeChildren() {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    StackChildrenVertically(children, PanelInnerContentRect(bounds, title, metrics), metrics.ControlGap());
}

void SparkPanel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect inner = PanelInnerContentRect(bounds, title, metrics);
    if (inner.width <= 0.0F || inner.height <= 0.0F) {
        return;
    }
    renderer.PushClip(inner);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
    renderer.PopClip();
}

void SparkPanel::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect b = GetBounds();
    renderer.FillDropShadow(
            b.x, b.y, b.width, b.height, metrics.Scaled(5.0F), metrics.Scaled(6.0F), theme.shadowRgb, 1.0F);
    renderer.FillRectGradientVertical(
            b.x, b.y, b.width, b.height, theme.panelElevatedTop, theme.panelElevatedBottom, theme.panelElevatedAlpha);
    renderer.StrokeRect(b.x, b.y, b.width, b.height, 1.0F, theme.borderRgb, 0.48F);
    if (!title.IsEmpty()) {
        renderer.DrawText(
                b.x + metrics.Padding(),
                b.y + metrics.Scaled(6.0F),
                b.width - metrics.Padding() * 2.0F,
                title,
                theme.labelPrimary,
                1.0F,
                metrics.FontLabel(),
                true);
    }
}

SparkLabel::SparkLabel(const LabelDesc& desc) : UiElementBase(desc.id), text(desc.text), muted(desc.muted) {
    SetHitTest(false);
}

void SparkLabel::SetText(Utf8String textIn) {
    text = MoveTemp(textIn);
}

void SparkLabel::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(80.0F));
    outDesired.height = ClampHeight(constraints, metrics.FontBody());
}

void SparkLabel::DoPaint(IUiRenderer& renderer) {
    if (text.IsEmpty()) {
        return;
    }
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Vector3 color = muted ? theme.labelMuted : theme.labelPrimary;
    const float fontPx = metrics.FontBody();
    TextLayout layout{};
    layout.wrap = TextWrap::WordWrap;
    renderer.DrawTextInRect(GetBounds(), fontPx, text, color, 1.0F, false, layout);
}

SparkSeparator::SparkSeparator(const SeparatorDesc& desc) : UiElementBase(desc.id) {
    SetHitTest(false);
}

void SparkSeparator::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(80.0F));
    outDesired.height = ClampHeight(constraints, metrics.Scaled(10.0F));
}

void SparkSeparator::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const Rect b = GetBounds();
    const float y = b.y + b.height * 0.5F;
    renderer.FillRect(b.x, y, b.width, 1.0F, theme.borderRgb, 0.45F);
}

SparkSlider::SparkSlider(const SliderDesc& desc)
    : UiElementBase(desc.id)
    , label(desc.label)
    , value(desc.value)
    , minValue(desc.minValue)
    , maxValue(desc.maxValue) {
    SetEnabled(desc.enabled);
}

void SparkSlider::SetValue(const float valueIn) {
    const float lo = minValue < maxValue ? minValue : maxValue;
    const float hi = minValue < maxValue ? maxValue : minValue;
    value = Clampf(valueIn, lo, hi);
}

void SparkSlider::SetRange(const float minValueIn, const float maxValueIn) {
    minValue = minValueIn;
    maxValue = maxValueIn;
    SetValue(value);
}

void SparkSlider::ApplyPointerX(const float mx) {
    const float lo = minValue < maxValue ? minValue : maxValue;
    const float hi = minValue < maxValue ? maxValue : minValue;
    const float span = GetBounds().width > 0.001F ? GetBounds().width : 0.001F;
    const float t = Clampf((mx - GetBounds().x) / span, 0.0F, 1.0F);
    const float next = lo + t * (hi - lo);
    if (std::fabs(next - value) > 1.0e-5F) {
        value = next;
        onChanged.Invoke(value);
    }
}

void SparkSlider::OnPointerDown(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!IsEnabled() || !GetBounds().Contains(input.mouseX, input.mouseY)) {
        return;
    }
    dragging = true;
    ApplyPointerX(input.mouseX);
}

void SparkSlider::OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!IsEnabled() || !dragging) {
        return;
    }
    ApplyPointerX(input.mouseX);
}

void SparkSlider::OnPointerUp(const UiFrameInput& /*input*/, UiCanvasComponent& /*canvas*/) {
    dragging = false;
}

void SparkSlider::ProcessKeyInput(IInput& input) {
    if (!IsEnabled()) {
        return;
    }
    const float lo = minValue < maxValue ? minValue : maxValue;
    const float hi = minValue < maxValue ? maxValue : minValue;
    const float span = hi - lo > 0.001F ? hi - lo : 0.001F;
    const float step = span * 0.02F;
    if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT) || input.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
        SetValue(value - step);
        onChanged.Invoke(value);
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT) || input.IsKeyPressedThisFrame(GLFW_KEY_UP)) {
        SetValue(value + step);
        onChanged.Invoke(value);
    }
}

void SparkSlider::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(180.0F));
    const float labelH = label.IsEmpty() ? 0.0F : metrics.FontSmall() + metrics.Scaled(2.0F);
    outDesired.height = ClampHeight(constraints, labelH + metrics.Scaled(28.0F));
}

void SparkSlider::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect b = GetBounds();
    float trackY = b.y;
    if (!label.IsEmpty()) {
        renderer.DrawText(b.x, b.y, b.width, label, theme.labelMuted, 1.0F, metrics.FontSmall(), false);
        trackY = b.y + metrics.FontSmall() + metrics.Scaled(4.0F);
    }
    const float trackH = metrics.Scaled(12.0F);
    const float lo = minValue < maxValue ? minValue : maxValue;
    const float hi = minValue < maxValue ? maxValue : minValue;
    const float t = hi > lo ? (value - lo) / (hi - lo) : 0.0F;
    renderer.FillRect(b.x, trackY, b.width, trackH, theme.sliderTrackRgb, 0.65F);
    const float knobW = metrics.Scaled(14.0F);
    const float knobX = b.x + t * b.width - knobW * 0.5F;
    const float knobY = trackY - metrics.Scaled(4.0F);
    const float knobH = trackH + metrics.Scaled(8.0F);
    renderer.FillDropShadow(knobX, knobY, knobW, knobH, metrics.Scaled(1.5F), metrics.Scaled(2.0F), theme.shadowRgb, 0.55F);
    renderer.FillRectGradientVertical(knobX, knobY, knobW, knobH, theme.sliderThumbTop, theme.sliderThumbBottom, 0.95F);
    renderer.StrokeRect(knobX, knobY, knobW, knobH, 1.0F, theme.borderRgb, 0.55F);
}

SparkCheckBox::SparkCheckBox(const CheckBoxDesc& desc)
    : UiElementBase(desc.id), label(desc.label), value(desc.value) {
    SetEnabled(desc.enabled);
}

void SparkCheckBox::OnPointerUp(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!IsEnabled() || !GetBounds().Contains(input.mouseX, input.mouseY)) {
        return;
    }
    value = !value;
    onChanged.Invoke(value);
}

void SparkCheckBox::ProcessKeyInput(IInput& input) {
    if (!IsEnabled()) {
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
        value = !value;
        onChanged.Invoke(value);
    }
}

void SparkCheckBox::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(160.0F));
    outDesired.height = ClampHeight(constraints, metrics.FormRowHeight());
}

void SparkCheckBox::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect b = GetBounds();
    const float drawFont = metrics.FontControl();
    const float box = metrics.Scaled(20.0F) > drawFont * 0.92F ? metrics.Scaled(20.0F) : drawFont * 0.92F;
    const float boxTop = b.y + (b.height - box) * 0.5F;
    renderer.FillRectGradientVertical(b.x, boxTop, box, box, theme.checkFrameTop, theme.checkFrameBottom, 0.92F);
    renderer.StrokeRect(b.x, boxTop, box, box, 1.0F, theme.borderRgb, 0.7F);
    if (value) {
        const float inset = box * 0.18F;
        renderer.FillRectGradientVertical(
                b.x + inset,
                boxTop + inset,
                box - 2.0F * inset,
                box - 2.0F * inset,
                theme.checkFillTop,
                theme.checkFillBottom,
                0.98F);
        renderer.StrokeRect(
                b.x + inset,
                boxTop + inset,
                box - 2.0F * inset,
                box - 2.0F * inset,
                1.0F,
                theme.checkInnerStrokeRgb,
                0.55F);
    }
    const float textY = b.y + (b.height - drawFont) * 0.5F;
    renderer.DrawText(b.x + box + metrics.Scaled(12.0F), textY, b.width - box - metrics.Scaled(12.0F), label, theme.labelPrimary, 1.0F, drawFont, false);
}

SparkDockWorkspace::SparkDockWorkspace(const DockWorkspaceDesc& desc)
    : UiElementBase(desc.id), leftWidth(desc.leftWidth), rightWidth(desc.rightWidth) {
    PanelDesc leftDesc{};
    leftDesc.id = Utf8String("dock.left");
    leftDesc.title = Utf8String("Left");
    PanelDesc centerDesc{};
    centerDesc.id = Utf8String("dock.center");
    centerDesc.title = Utf8String("Center");
    PanelDesc rightDesc{};
    rightDesc.id = Utf8String("dock.right");
    rightDesc.title = Utf8String("Right");

    auto left = MakeUnique<SparkPanel>(leftDesc);
    auto center = MakeUnique<SparkPanel>(centerDesc);
    auto right = MakeUnique<SparkPanel>(rightDesc);
    leftPane = left.Get();
    centerPane = center.Get();
    rightPane = right.Get();
    centerPane->SetHitTest(false);
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(left.Release())));
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(center.Release())));
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(right.Release())));
}

float SparkDockWorkspace::EffectiveLeftWidth() const noexcept {
    return leftCollapsed ? 0.0F : leftWidth;
}

float SparkDockWorkspace::EffectiveRightWidth() const noexcept {
    return rightCollapsed ? 0.0F : rightWidth;
}

void SparkDockWorkspace::ToggleLeftCollapsed() noexcept {
    leftCollapsed = !leftCollapsed;
}

void SparkDockWorkspace::ToggleRightCollapsed() noexcept {
    rightCollapsed = !rightCollapsed;
}

void SparkDockWorkspace::SetLeftWidth(const float width) noexcept {
    leftWidth = std::max(160.0F, width);
}

void SparkDockWorkspace::SetRightWidth(const float width) noexcept {
    rightWidth = std::max(200.0F, width);
}

void SparkDockWorkspace::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    outDesired.width = ClampWidth(constraints, leftWidth + rightWidth + 200.0F);
    outDesired.height = ClampHeight(constraints, 240.0F);
}

void SparkDockWorkspace::DoArrangeChildren() {
    const Rect b = GetBounds();
    const float effLeft = EffectiveLeftWidth();
    const float effRight = EffectiveRightWidth();
    const float centerWidth = std::max(0.0F, b.width - effLeft - effRight);
    if (children.GetSize() >= 3U) {
        if (children[0] != nullptr) {
            children[0]->Arrange(Rect{b.x, b.y, effLeft, b.height});
            if (leftPane != nullptr) {
                leftPane->SetVisible(effLeft > 0.0F);
            }
        }
        if (children[1] != nullptr) {
            const Rect centerRect{b.x + effLeft, b.y, centerWidth, b.height};
            children[1]->Arrange(centerRect);
            centerBounds = centerRect;
        }
        if (children[2] != nullptr) {
            children[2]->Arrange(Rect{b.x + effLeft + centerWidth, b.y, effRight, b.height});
            if (rightPane != nullptr) {
                rightPane->SetVisible(effRight > 0.0F);
            }
        }
    }
}

void SparkDockWorkspace::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const Rect b = GetBounds();
    renderer.FillRect(b.x, b.y, b.width, b.height, theme.panelElevatedBottom, theme.panelElevatedAlpha * 0.25F);
}

}  // namespace Spark::Ui
