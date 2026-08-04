#pragma once

#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark::Ui {

class ImguiUiRenderer;

/** Dear ImGui retained controls — widgets are emitted during <c>Paint</c> (Phase 3). */
class ImguiButton final : public IButton, public UiElementBase {
public:
    explicit ImguiButton(const ButtonDesc& desc);

    void SetLabel(Utf8String labelIn) override;
    [[nodiscard]] Utf8StringView GetLabel() const noexcept override { return label; }
    void SetOnClick(UiVoidCallback handler) override { onClick = handler; }
    [[nodiscard]] bool WasClickedThisFrame() const noexcept override { return clickedThisFrame; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String label{};
    UiVoidCallback onClick{};
    bool clickedThisFrame = false;
};

class ImguiPanel final : public IPanel, public UiElementBase {
public:
    explicit ImguiPanel(const PanelDesc& desc);

    void SetTitle(Utf8String titleIn) override;
    [[nodiscard]] bool IsOpen() const noexcept override { return open; }

    void Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String title{};
    bool open = true;
    float designWidth = 0.0F;
    float designHeight = 0.0F;
    bool anchorRight = false;
    float edgeMargin = 8.0F;
    bool centerInParent = false;
    bool collapsible = false;
};

class ImguiLabel final : public ILabel, public UiElementBase {
public:
    explicit ImguiLabel(const LabelDesc& desc);
    void SetText(Utf8String textIn) override;
    void SetMuted(bool mutedIn) override { muted = mutedIn; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String text{};
    bool muted = false;
};

class ImguiSeparator final : public ISeparator, public UiElementBase {
public:
    explicit ImguiSeparator(const SeparatorDesc& desc);

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;
};

class ImguiScrollPanel final : public IScrollPanel, public UiElementBase {
public:
    explicit ImguiScrollPanel(const ScrollPanelDesc& desc);
    void SetScrollY(float y) noexcept override;
    [[nodiscard]] float GetScrollY() const noexcept override;
    void ScrollToTop() noexcept override;

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    float designHeight = 240.0F;
    float scrollY = 0.0F;
};

class ImguiSlider final : public ISlider, public UiElementBase {
public:
    explicit ImguiSlider(const SliderDesc& desc);
    void SetValue(float valueIn) override { value = valueIn; }
    [[nodiscard]] float GetValue() const noexcept override { return value; }
    void SetRange(float minValueIn, float maxValueIn) override {
        minValue = minValueIn;
        maxValue = maxValueIn;
    }
    void SetOnChanged(UiFloatCallback handler) override { onChanged = handler; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String label{};
    float value = 0.0F;
    float minValue = 0.0F;
    float maxValue = 1.0F;
    UiFloatCallback onChanged{};
};

class ImguiCheckBox final : public ICheckBox, public UiElementBase {
public:
    explicit ImguiCheckBox(const CheckBoxDesc& desc);
    void SetValue(bool valueIn) override { value = valueIn; }
    [[nodiscard]] bool GetValue() const noexcept override { return value; }
    void SetOnChanged(UiBoolCallback handler) override { onChanged = handler; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String label{};
    bool value = false;
    UiBoolCallback onChanged{};
};

class ImguiDockWorkspace final : public IDockWorkspace, public UiElementBase {
public:
    explicit ImguiDockWorkspace(const DockWorkspaceDesc& desc);
    IUiElement* GetLeftPane() noexcept override { return leftPane; }
    IUiElement* GetCenterPane() noexcept override { return centerPane; }
    IUiElement* GetRightPane() noexcept override { return rightPane; }

    void ToggleLeftCollapsed() noexcept override;
    void ToggleRightCollapsed() noexcept override;
    [[nodiscard]] Rect GetCenterBounds() const noexcept override { return centerBounds; }
    void SetLeftWidth(float width) noexcept override;
    [[nodiscard]] float GetLeftWidth() const noexcept override { return leftWidth; }
    void SetRightWidth(float width) noexcept override;
    [[nodiscard]] float GetRightWidth() const noexcept override { return rightWidth; }

    void Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;

protected:
    void DoPaint(IUiRenderer& renderer) override;

private:
    [[nodiscard]] float EffectiveLeftWidth() const noexcept;
    [[nodiscard]] float EffectiveRightWidth() const noexcept;

    ImguiPanel* leftPane = nullptr;
    ImguiPanel* centerPane = nullptr;
    ImguiPanel* rightPane = nullptr;
    Rect centerBounds{};
    float leftWidth = 280.0F;
    float rightWidth = 320.0F;
    bool leftCollapsed = false;
    bool rightCollapsed = false;
    bool dockLayoutBuilt = false;
    char leftWindowName[192]{};
    char centerWindowName[192]{};
    char rightWindowName[192]{};
};

}  // namespace Spark::Ui
