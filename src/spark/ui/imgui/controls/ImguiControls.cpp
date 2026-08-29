#include "spark/ui/imgui/controls/ImguiControls.hpp"

#include "spark/config.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/ImguiUiRenderer.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

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

[[nodiscard]] bool IsEditorDockRoot(const IUiElement* element) noexcept {
    return element != nullptr && element->GetId() == Utf8String("editor_dock_root");
}

[[nodiscard]] const IUiElement* FindDockHostAncestor(const IUiElement* element) noexcept {
    const IUiElement* current = element != nullptr ? element->GetParent() : nullptr;
    while (current != nullptr) {
        if (IsEditorDockRoot(current)) {
            return current;
        }
        if (dynamic_cast<const IDockWorkspace*>(current) != nullptr) {
            return current;
        }
        current = current->GetParent();
    }
    return nullptr;
}

[[nodiscard]] bool IsDirectDockHostChild(const IUiElement* element) noexcept {
    const IUiElement* parent = element != nullptr ? element->GetParent() : nullptr;
    if (parent == nullptr) {
        return false;
    }
    return IsEditorDockRoot(parent) || dynamic_cast<const IDockWorkspace*>(parent) != nullptr;
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
    , collapsible(desc.collapsible)
    , horizontalLayout(desc.horizontalLayout) {}

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
    if (IsDirectDockHostChild(this)) {
        bounds = finalBounds;
        DoArrangeChildren();
        return;
    }
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
    DoArrangeChildren();
}

void ImguiPanel::ArrangeChildrenToImGuiContent() {
#if SPARK_ENABLE_IMGUI
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const Rect content{pos.x, pos.y, avail.x, avail.y};
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Arrange(content);
        }
    }
#else
    DoArrangeChildren();
#endif
}

void ImguiPanel::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr) {
        return;
    }

    if (FindDockHostAncestor(this) != nullptr && !IsDirectDockHostChild(this)) {
        UiElementBase::Paint(renderer);
        return;
    }

    Rect panelBounds = GetBounds();
    bool* openPtr = collapsible ? &open : nullptr;
    ImguiPanelPlacement placement = ImguiPanelPlacement::Movable;
    if (IsDirectDockHostChild(this)) {
        panelBounds = {};
        placement = children.GetSize() == 0U ? ImguiPanelPlacement::DockedPassthrough : ImguiPanelPlacement::Docked;
    } else if (centerInParent) {
        placement = ImguiPanelPlacement::CenterOnce;
    } else if (panelBounds.width > 1.0F && panelBounds.height > 1.0F) {
        placement = ImguiPanelPlacement::LockedSide;
    }

    if (!imgui->BeginPanel(GetId().CStr(), title, openPtr, panelBounds, placement)) {
        return;
    }
    ArrangeChildrenToImGuiContent();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            if (horizontalLayout && i > 0U) {
                imgui->SameLine();
            }
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

ImguiScrollPanel::ImguiScrollPanel(const ScrollPanelDesc& desc)
    : UiElementBase(desc.id), designHeight(desc.height), fillRemainingHeight(desc.fillRemainingHeight) {}

void ImguiScrollPanel::SetScrollY(const float y) noexcept {
    scrollY = y;
    applyStoredScroll = true;
}

float ImguiScrollPanel::GetScrollY() const noexcept {
    return scrollY;
}

void ImguiScrollPanel::ScrollToTop() noexcept {
    scrollY = 0.0F;
    applyStoredScroll = true;
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
    const float height = fillRemainingHeight ? 0.0F : metrics.Scaled(designHeight);
    if (!imgui->BeginScrollRegion(GetId().CStr(), height)) {
        return;
    }
    if (applyStoredScroll) {
        imgui->SetScrollY(scrollY);
        applyStoredScroll = false;
    }
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
    : UiElementBase(desc.id)
    , label(desc.label)
    , value(desc.value)
    , minValue(desc.minValue)
    , maxValue(desc.maxValue)
    , dragInput(desc.dragInput) {
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
    bool changed = false;
    if (dragInput) {
        const float range = (std::max)(maxValue - minValue, 0.01F);
        const float speed = range * 0.005F;
        changed = imgui->DragFloat(GetId().CStr(), drawLabel, value, speed, minValue, maxValue);
    } else {
        changed = imgui->SliderFloat(GetId().CStr(), drawLabel, value, minValue, maxValue);
    }
    if (changed) {
        if (onChanged.fn != nullptr) {
            onChanged.fn(onChanged.userData, value);
        }
    }
#if SPARK_ENABLE_IMGUI
    else if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (onChanged.fn != nullptr) {
            onChanged.fn(onChanged.userData, value);
        }
    }
#endif
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

ImguiTextBox::ImguiTextBox(const TextFieldDesc& desc)
    : UiElementBase(desc.id), label(desc.label), text(desc.text) {
    SetEnabled(desc.enabled);
}

void ImguiTextBox::DoPaint(IUiRenderer& renderer) {
    ImguiUiRenderer* imgui = AsImguiRenderer(renderer);
    if (imgui == nullptr || !IsEnabled()) {
        return;
    }

#if SPARK_ENABLE_IMGUI
    ImGui::PushID(GetId().CStr());
    if (!label.IsEmpty()) {
        ImGui::TextUnformatted(label.CStr());
    }

    char buffer[256]{};
    if (const char* current = text.CStr()) {
        std::strncpy(buffer, current, sizeof(buffer) - 1U);
    }

    editing = ImGui::InputText("##value", buffer, sizeof(buffer));
    text = Utf8String(buffer);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editing = false;
        onCommit.Invoke();
    } else {
        editing = ImGui::IsItemActive();
    }
    ImGui::PopID();
#else
    (void)imgui;
#endif
}

void ImguiTextBox::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
}

ImguiDockWorkspace::ImguiDockWorkspace(const DockWorkspaceDesc& desc)
    : UiElementBase(desc.id)
    , leftWidth(desc.leftWidth)
    , rightWidth(desc.rightWidth)
    , enableDockBuilder(desc.enableDockBuilder) {
    PanelDesc leftDesc{};
    leftDesc.id = Utf8String("dock.left");
    leftDesc.title = desc.leftTitle.IsEmpty() ? Utf8String("Hierarchy") : desc.leftTitle;
    PanelDesc centerDesc{};
    centerDesc.id = Utf8String("dock.center");
    centerDesc.title = desc.centerTitle.IsEmpty() ? Utf8String("Scene") : desc.centerTitle;
    PanelDesc rightDesc{};
    rightDesc.id = Utf8String("dock.right");
    rightDesc.title = desc.rightTitle.IsEmpty() ? Utf8String("Inspector") : desc.rightTitle;

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
    if (children.GetSize() >= 3U) {
        if (children[0] != nullptr) {
            children[0]->Arrange(Rect{finalBounds.x, finalBounds.y, effLeft, finalBounds.height});
        }
        if (children[1] != nullptr) {
            children[1]->Arrange(Rect{finalBounds.x + effLeft, finalBounds.y, centerWidth, finalBounds.height});
        }
        if (children[2] != nullptr) {
            children[2]->Arrange(
                    Rect{finalBounds.x + effLeft + centerWidth, finalBounds.y, effRight, finalBounds.height});
        }
    }
}

void ImguiDockWorkspace::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    if (!enableDockBuilder) {
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                children[i]->Paint(renderer);
            }
        }
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
#if SPARK_ENABLE_IMGUI
    if (ImGuiWindow* centerWindow = ImGui::FindWindowByName(centerWindowName)) {
        centerBounds = Rect{centerWindow->Pos.x, centerWindow->Pos.y, centerWindow->Size.x, centerWindow->Size.y};
    }
#endif
    imgui->EndDockWorkspace();
}

void ImguiDockWorkspace::DoPaint(IUiRenderer& /*renderer*/) {}

}  // namespace Spark::Ui
