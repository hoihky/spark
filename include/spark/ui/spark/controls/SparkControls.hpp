#pragma once

#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

#include "spark/engine/IInput.hpp"

namespace Spark {

class UiCanvasComponent;

namespace Ui {

class SparkButton final : public IButton, public UiElementBase {
public:
    explicit SparkButton(const ButtonDesc& desc);

    void SetLabel(Utf8String labelIn) override;
    [[nodiscard]] Utf8StringView GetLabel() const noexcept override { return label; }
    void SetOnClick(UiVoidCallback handler) override { onClick = handler; }
    /** When set, pointer-up invokes this instead of <c>SetOnClick</c> (modifiers + canvas for list rows). */
    void SetOnClickWithFrame(UiFrameVoidCallback handler) noexcept { onClickWithFrame = handler; }
    void ClearOnClickWithFrame() noexcept { onClickWithFrame = {}; }
    [[nodiscard]] bool WasClickedThisFrame() const noexcept override { return clickedThisFrame; }

    void SetFontSize(float px) noexcept { fontPx = px; }
    void SetLabelBold(bool boldIn) noexcept { labelBold = boldIn; }
    void SetAccentSelected(bool selected) noexcept { accentSelected = selected; }
    void SetOpaqueSurface(bool opaque) noexcept { opaqueSurface = opaque; }

    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String label{};
    UiVoidCallback onClick{};
    UiFrameVoidCallback onClickWithFrame{};
    bool clickedThisFrame = false;
    float fontPx = 0.0F;
    bool labelBold = false;
    bool accentSelected = false;
    bool opaqueSurface = false;
};

class SparkPanel final : public IPanel, public UiElementBase {
public:
    explicit SparkPanel(const PanelDesc& desc);

    void SetTitle(Utf8String titleIn) override;
    [[nodiscard]] bool IsOpen() const noexcept override { return open; }

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void Arrange(const Rect& finalBounds) override;
    void DoArrangeChildren() override;
    void Paint(IUiRenderer& renderer) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String title{};
    bool open = true;
    float designWidth = 0.0F;
    float designHeight = 0.0F;
    bool anchorRight = false;
    float edgeMargin = 8.0F;
    bool centerInParent = false;
};

class SparkLabel final : public ILabel, public UiElementBase {
public:
    explicit SparkLabel(const LabelDesc& desc);

    void SetText(Utf8String textIn) override;
    void SetMuted(bool mutedIn) override { muted = mutedIn; }

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String text{};
    bool muted = false;
};

class SparkSeparator final : public ISeparator, public UiElementBase {
public:
    explicit SparkSeparator(const SeparatorDesc& desc);

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;
};

class SparkSlider final : public ISlider, public UiElementBase {
public:
    explicit SparkSlider(const SliderDesc& desc);

    void SetValue(float valueIn) override;
    [[nodiscard]] float GetValue() const noexcept override { return value; }
    void SetRange(float minValueIn, float maxValueIn) override;
    void SetOnChanged(UiFloatCallback handler) override { onChanged = handler; }

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;
    void OnPointerDown(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    void ApplyPointerX(float mx);

    Utf8String label{};
    float value = 0.0F;
    float minValue = 0.0F;
    float maxValue = 1.0F;
    UiFloatCallback onChanged{};
    bool dragging = false;
};

class SparkCheckBox final : public ICheckBox, public UiElementBase {
public:
    explicit SparkCheckBox(const CheckBoxDesc& desc);

    void SetValue(bool valueIn) override { value = valueIn; }
    [[nodiscard]] bool GetValue() const noexcept override { return value; }
    void SetOnChanged(UiBoolCallback handler) override { onChanged = handler; }

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;
    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    Utf8String label{};
    bool value = false;
    UiBoolCallback onChanged{};
};

class SparkDockWorkspace final : public IDockWorkspace, public UiElementBase {
public:
    explicit SparkDockWorkspace(const DockWorkspaceDesc& desc);

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

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoArrangeChildren() override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    [[nodiscard]] float EffectiveLeftWidth() const noexcept;
    [[nodiscard]] float EffectiveRightWidth() const noexcept;

    SparkPanel* leftPane = nullptr;
    SparkPanel* centerPane = nullptr;
    SparkPanel* rightPane = nullptr;
    Rect centerBounds{};
    float leftWidth = 280.0F;
    float rightWidth = 320.0F;
    bool leftCollapsed = false;
    bool rightCollapsed = false;
};

}  // namespace Ui
}  // namespace Spark
