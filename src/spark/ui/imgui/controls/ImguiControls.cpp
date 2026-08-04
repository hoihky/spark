#include "spark/ui/imgui/controls/ImguiControls.hpp"

#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/ImguiUiRenderer.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"

#include <algorithm>
#include <cstdio>

namespace Spark::Ui {

namespace {

[[nodiscard]] ImguiUiRenderer* AsImguiRenderer(IUiRenderer& renderer) noexcept {
    return dynamic_cast<ImguiUiRenderer*>(&renderer);
}

float ClampWidthMeasure(const UiMeasureConstraints& constraints, const float desired) {
    float w = desired;
    if (constraints.maxWidth > 0.0F) {
        w = (std::min)(w, constraints.maxWidth);
    }
    return (std::max)(w, constraints.minWidth);
}

float ClampHeightMeasure(const UiMeasureConstraints& constraints, const float desired) {
    float h = desired;
    if (constraints.maxHeight > 0.0F) {
        h = (std::min)(h, constraints.maxHeight);
    }
    return (std::max)(h, constraints.minHeight);
}

void FormatDockWindowName(const char* id, const char* title, char* out, const std::size_t outSize) {
    std::snprintf(out, outSize, "%s###%s", title, id);
}

}  // namespace

ImguiButton::ImguiButton(const ButtonDesc& desc) : UiElementBase(desc.id), label(desc.label) {
    SetEnabled(desc.enabled);
}

void ImguiButton::SetLabel(Utf8String labelIn) {
    label = MoveTemp(labelIn);
}

void ImguiButton::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

void ImguiButton::DoPaint(IUiRenderer& renderer) {
    clickedThisFrame = false;
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr || !IsEnabled()) {
        return;
    }
    const Utf8StringView drawLabel = label.IsEmpty() ? Utf8StringView(GetId().CStr()) : Utf8StringView(label);
    if (imgui->Button(GetId().CStr(), drawLabel)) {
        clickedThisFrame = true;
        onClick.Invoke();
    }
}

ImguiPanel::ImguiPanel(const PanelDesc& desc)
    : UiElementBase(desc.id)
    , title(desc.title)
    , open(desc.open)
    , designWidth(desc.width)
    , designHeight(desc.height)
    , anchorRight(desc.anchorRight)
    , edgeMargin(desc.edgeMargin)
    , centerInParent(desc.centerInParent)
    , collapsible(desc.collapsible) {}

void ImguiPanel::SetTitle(Utf8String titleIn) {
    title = MoveTemp(titleIn);
}

void ImguiPanel::Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float pad = metrics.Padding();
    float w = designWidth > 0.0F ? metrics.Scaled(designWidth) : constraints.maxWidth;
    float h = designHeight > 0.0F ? metrics.Scaled(designHeight) : constraints.maxHeight;
    outDesired.width = (std::max)(constraints.minWidth, w > 0.0F ? w : pad * 2.0F + 120.0F);
    outDesired.height = (std::max)(constraints.minHeight, h > 0.0F ? h : metrics.Scaled(200.0F));
}

void ImguiPanel::Arrange(const Rect& finalBounds) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float margin = metrics.Scaled(edgeMargin);
    float w = designWidth > 0.0F ? metrics.Scaled(designWidth) : finalBounds.width;
    float h = designHeight > 0.0F ? metrics.Scaled(designHeight) : finalBounds.height - margin * 2.0F;
    const float maxW = std::max(0.0F, finalBounds.width - margin * 2.0F);
    const float maxH = std::max(0.0F, finalBounds.height - margin * 2.0F);
    if (designWidth > 0.0F) {
        w = (std::min)(w, maxW);
    }
    if (designHeight > 0.0F) {
        h = (std::min)(h, maxH);
    } else {
        h = (std::min)(h, maxH);
    }
    const float minW = maxW > 0.0F ? (std::min)(160.0F, maxW) : 160.0F;
    const float minH = maxH > 0.0F ? (std::min)(120.0F, maxH) : 120.0F;
    w = (std::max)(w, minW);
    h = (std::max)(h, minH);
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
}

void ImguiPanel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr) {
        return;
    }
    Rect panelBounds{};
    if (dynamic_cast<const IDockWorkspace*>(GetParent()) == nullptr) {
        panelBounds = GetBounds();
    }
    bool* openPtr = collapsible ? &open : nullptr;
    ImguiPanelPlacement placement = ImguiPanelPlacement::Movable;
    if (centerInParent) {
        placement = ImguiPanelPlacement::CenterOnce;
    }
    if (!imgui->BeginPanel(GetId().CStr(), title, openPtr, panelBounds, placement)) {
        return;
    }
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
    imgui->EndPanel();
}

void ImguiPanel::DoPaint(IUiRenderer& /*renderer*/) {}

ImguiLabel::ImguiLabel(const LabelDesc& desc) : UiElementBase(desc.id), text(desc.text), muted(desc.muted) {
    SetHitTest(false);
}

void ImguiLabel::SetText(Utf8String textIn) {
    text = MoveTemp(textIn);
}

void ImguiLabel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

void ImguiLabel::DoPaint(IUiRenderer& renderer) {
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr || text.IsEmpty()) {
        return;
    }
    if (muted) {
        imgui->TextDisabled(text);
    } else {
        imgui->TextUnformatted(text);
    }
}

ImguiSeparator::ImguiSeparator(const SeparatorDesc& desc) : UiElementBase(desc.id) {
    SetHitTest(false);
}

void ImguiSeparator::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

void ImguiSeparator::DoPaint(IUiRenderer& renderer) {
    if (ImguiUiRenderer* imgui = AsImguiRenderer(renderer)) {
        imgui->Separator();
    }
}

ImguiScrollPanel::ImguiScrollPanel(const ScrollPanelDesc& desc) : UiElementBase(desc.id), designHeight(desc.height) {}

void ImguiScrollPanel::SetScrollY(const float y) noexcept {
    scrollY = y;
}

float ImguiScrollPanel::GetScrollY() const noexcept {
    return scrollY;
}

void ImguiScrollPanel::ScrollToTop() noexcept {
    scrollY = 0.0F;
}

void ImguiScrollPanel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr) {
        return;
    }
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const float height = metrics.Scaled(designHeight);
    if (!imgui->BeginScrollRegion(GetId().CStr(), height)) {
        return;
    }
    imgui->SetScrollY(scrollY);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
    scrollY = imgui->GetScrollY();
    imgui->EndScrollRegion();
}

void ImguiScrollPanel::DoPaint(IUiRenderer& /*renderer*/) {}

ImguiSlider::ImguiSlider(const SliderDesc& desc)
    : UiElementBase(desc.id), label(desc.label), value(desc.value), minValue(desc.minValue), maxValue(desc.maxValue) {
    SetEnabled(desc.enabled);
}

void ImguiSlider::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

void ImguiSlider::DoPaint(IUiRenderer& renderer) {
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr || !IsEnabled()) {
        return;
    }
    const Utf8StringView drawLabel = label.IsEmpty() ? Utf8StringView(GetId().CStr()) : Utf8StringView(label);
    if (imgui->SliderFloat(GetId().CStr(), drawLabel, value, minValue, maxValue)) {
        if (onChanged.fn != nullptr) {
            onChanged.fn(onChanged.userData, value);
        }
    }
}

ImguiCheckBox::ImguiCheckBox(const CheckBoxDesc& desc) : UiElementBase(desc.id), label(desc.label), value(desc.value) {
    SetEnabled(desc.enabled);
}

void ImguiCheckBox::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

void ImguiCheckBox::DoPaint(IUiRenderer& renderer) {
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr || !IsEnabled()) {
        return;
    }
    const Utf8StringView drawLabel = label.IsEmpty() ? Utf8StringView(GetId().CStr()) : Utf8StringView(label);
    if (imgui->Checkbox(GetId().CStr(), drawLabel, value)) {
        if (onChanged.fn != nullptr) {
            onChanged.fn(onChanged.userData, value);
        }
    }
}

ImguiDockWorkspace::ImguiDockWorkspace(const DockWorkspaceDesc& desc)
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

    FormatDockWindowName(leftDesc.id.CStr(), leftDesc.title.CStr(), leftWindowName, sizeof(leftWindowName));
    FormatDockWindowName(centerDesc.id.CStr(), centerDesc.title.CStr(), centerWindowName, sizeof(centerWindowName));
    FormatDockWindowName(rightDesc.id.CStr(), rightDesc.title.CStr(), rightWindowName, sizeof(rightWindowName));

    auto left = MakeUnique<ImguiPanel>(leftDesc);
    auto center = MakeUnique<ImguiPanel>(centerDesc);
    auto right = MakeUnique<ImguiPanel>(rightDesc);
    leftPane = left.Get();
    centerPane = center.Get();
    rightPane = right.Get();
    centerPane->SetHitTest(false);
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(left.Release())));
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(center.Release())));
    AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(right.Release())));
}

float ImguiDockWorkspace::EffectiveLeftWidth() const noexcept {
    return leftCollapsed ? 0.0F : leftWidth;
}

float ImguiDockWorkspace::EffectiveRightWidth() const noexcept {
    return rightCollapsed ? 0.0F : rightWidth;
}

void ImguiDockWorkspace::ToggleLeftCollapsed() noexcept {
    leftCollapsed = !leftCollapsed;
    dockLayoutBuilt = false;
}

void ImguiDockWorkspace::ToggleRightCollapsed() noexcept {
    rightCollapsed = !rightCollapsed;
    dockLayoutBuilt = false;
}

void ImguiDockWorkspace::SetLeftWidth(const float width) noexcept {
    leftWidth = std::max(160.0F, width);
    dockLayoutBuilt = false;
}

void ImguiDockWorkspace::SetRightWidth(const float width) noexcept {
    rightWidth = std::max(200.0F, width);
    dockLayoutBuilt = false;
}

void ImguiDockWorkspace::Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    outDesired.width = ClampWidthMeasure(constraints, leftWidth + rightWidth + 200.0F);
    outDesired.height = ClampHeightMeasure(constraints, 240.0F);
}

void ImguiDockWorkspace::Arrange(const Rect& finalBounds) {
    bounds = finalBounds;
    const float effLeft = EffectiveLeftWidth();
    const float effRight = EffectiveRightWidth();
    const float centerWidth = std::max(0.0F, finalBounds.width - effLeft - effRight);
    centerBounds = Rect{finalBounds.x + effLeft, finalBounds.y, centerWidth, finalBounds.height};
}

void ImguiDockWorkspace::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr) {
        return;
    }
    if (!imgui->BeginDockWorkspace(
                GetId().CStr(),
                GetBounds(),
                EffectiveLeftWidth(),
                EffectiveRightWidth(),
                leftWindowName,
                centerWindowName,
                rightWindowName,
                dockLayoutBuilt)) {
        return;
    }
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
    imgui->EndDockWorkspace();
}

void ImguiDockWorkspace::DoPaint(IUiRenderer& /*renderer*/) {}

}  // namespace Spark::Ui
