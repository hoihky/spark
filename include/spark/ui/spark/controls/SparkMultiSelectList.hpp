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

/**
 * Scrollable list with Ctrl/Shift multi-selection (see <c>UiFrameInput</c> on row click).
 * Row visuals mirror <c>SparkList</c>; selection is tracked as a sorted unique index set.
 */
class SparkMultiSelectList final : public IMultiSelectList, public UiElementBase {
public:
    struct RowBinding {
        SparkMultiSelectList* list = nullptr;
        int index = -1;
    };

    explicit SparkMultiSelectList(const MultiSelectListDesc& desc);

    void SetItems(Array<Utf8String> itemsIn) override;
    [[nodiscard]] const Array<int>& GetSelectedIndices() const noexcept override { return selected; }
    void SetSelectedIndices(Array<int> indices) override;
    void SetOnSelectionChanged(UiVoidCallback handler) override { onSelect = handler; }
    void SetScrollY(float y) override;
    [[nodiscard]] float GetScrollY() const noexcept override { return scrollY; }

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

    void HandleRowClick(int index, const UiFrameInput& input);

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    void RebuildRows();
    void SyncScrollLayout() noexcept;
    [[nodiscard]] float ScaledRowHeight() const noexcept;
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void SortUniqueSelected();
    [[nodiscard]] bool IsSelected(int idx) const noexcept;
    void SetRowSelected(int idx, bool sel);
    void NotifySelectionChanged();
    [[nodiscard]] int VisibleRowCount() const noexcept { return static_cast<int>(items.GetSize()); }
    void ScrollSelectionIntoView(int idx) noexcept;
    void ApplyScrollWheelDelta(float deltaY) noexcept;

    float rowHeight = 0.0F;
    float itemFontPx = 0.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    bool verticalScrollingEnabled = true;
    Array<Utf8String> items{};
    Array<int> selected{};
    Array<RowBinding> rowBindings{};
    int anchorIndex = 0;
    UiVoidCallback onSelect{};

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
