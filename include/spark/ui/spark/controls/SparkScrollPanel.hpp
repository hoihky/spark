#pragma once

#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark {

class UiCanvasComponent;

namespace Ui {

/** Vertical stack with scroll offset and an integrated scrollbar strip on the right. */
class SparkScrollPanel final : public IScrollPanel, public UiElementBase {
public:
    explicit SparkScrollPanel(const ScrollPanelDesc& desc);

    void SetScrollY(float y) noexcept override;
    [[nodiscard]] float GetScrollY() const noexcept override { return scrollY; }
    void ScrollToTop() noexcept override;

    [[nodiscard]] bool WantsScrollInput() const override { return true; }

    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;

    [[nodiscard]] IUiElement* HitTest(float x, float y) override;
    [[nodiscard]] const IUiElement* HitTest(float x, float y) const override;

    void OnPointerDown(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnScroll(float deltaX, float deltaY) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void SyncScrollLayout() noexcept;
    void ApplyScrollWheelDelta(float deltaY) noexcept;

    float designHeight = 240.0F;
    float rowHeight = 0.0F;
    float vGap = 4.0F;
    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;
    Rect trackRect{};
    Rect thumbRect{};
    Rect arrangeRect{};
    static constexpr float kTrackWidth = 12.0F;
};

}  // namespace Ui
}  // namespace Spark
