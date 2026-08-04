#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark {

class IInput;
class UiCanvasComponent;

namespace Ui {

class SparkList final : public IList, public UiElementBase {
public:
    struct RowBinding {
        SparkList* list = nullptr;
        int index = -1;
    };

    explicit SparkList(const ListDesc& desc);

    void SetItems(Array<Utf8String> itemsIn) override;
    [[nodiscard]] int GetSelectedIndex() const noexcept override { return selectedIndex; }
    void SetSelectedIndex(int index) override;
    void SetOnSelectionChanged(UiIntCallback handler) override { onSelect = handler; }
    void SetScrollY(float y) override;
    [[nodiscard]] float GetScrollY() const noexcept override { return scrollY; }
    void ScrollToTop() noexcept override;

    [[nodiscard]] bool WantsScrollInput() const override { return verticalScrollingEnabled; }
    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;
    void OnScroll(float deltaX, float deltaY) override;

    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;
    [[nodiscard]] IUiElement* HitTest(float x, float y) override;
    void OnPointerDown(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;

    void HandleRowClick(int index);

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    void RebuildRows();
    void SyncScrollLayout() noexcept;
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    [[nodiscard]] bool RowIntersectsViewport(const Rect& rowBounds) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void SnapScrollToRowGrid(float scaledRowH) noexcept;
    void ScrollSelectedIndexIntoView() noexcept;
    void ApplyScrollWheelDelta(float deltaY) noexcept;

    float rowHeight = 30.0F;
    float itemFontPx = 0.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    bool verticalScrollingEnabled = true;
    Array<Utf8String> items{};
    int selectedIndex = -1;
    UiIntCallback onSelect{};
    Array<RowBinding> rowBindings{};

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
