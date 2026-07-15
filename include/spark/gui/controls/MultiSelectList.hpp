#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class IInput;
class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

/**
 * Scrollable list with Ctrl/Shift multi-selection (see <c>GuiFrameInput</c> on row click).
 * Row visuals mirror <c>List</c>; selection is tracked as a sorted unique index set.
 */
class MultiSelectList final : public Widget {
public:
    MultiSelectList();
    void SetRowHeight(float h) noexcept { rowHeight = h; }
    void SetItemFontSize(float px) noexcept { itemFontPx = px; }
    void SetItemBold(bool b) noexcept;
    void SetOpaqueRows(bool v) noexcept;
    void SetItems(Array<Utf8String> lines);
    [[nodiscard]] const Array<int>& GetSelectedIndices() const noexcept { return selected; }
    void SetSelectedIndices(Array<int> indices);
    void SetOnSelectionChanged(std::function<void(const Array<int>&)> fn) {
        onSelect = Spark::MoveTemp(fn);
    }

    void SetScrollY(float y) noexcept { scrollY = y; }
    [[nodiscard]] float GetScrollY() const noexcept { return scrollY; }
    void SetVerticalScrollingEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IsVerticalScrollingEnabled() const noexcept { return verticalScrollingEnabled; }

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void ProcessKeyInput(IInput& input) override;

    void ApplyScrollWheelDelta(float deltaY) noexcept;

private:
    void Rebuild();
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void SortUniqueSelected();
    [[nodiscard]] bool IsSelected(int idx) const noexcept;
    void SetRowSelected(int idx, bool sel);
    void HandleRowClick(int idx, const GuiFrameInput& in);
    void NotifySelectionChanged();
    [[nodiscard]] int VisibleRowCount() const noexcept { return static_cast<int>(items.GetSize()); }
    void ScrollSelectionIntoView(int idx) noexcept;

    float rowHeight = 30.0F;
    float itemFontPx = 24.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    Array<Utf8String> items{};
    Array<int> selected{};
    int anchorIndex = 0;
    std::function<void(const Array<int>&)> onSelect{};

    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;
    Rect trackRect{};
    Rect thumbRect{};
    static constexpr float kTrackW = 12.0F;
    bool verticalScrollingEnabled = true;
};

}  // namespace Gui
}  // namespace Spark
