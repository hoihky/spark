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
 * Vertical list of selectable string rows. When content is taller than the arranged height, rows scroll vertically
 * with an integrated scrollbar (same strip behavior as <c>ScrollPanel</c>) and mouse wheel when supported.
 */
class List final : public Widget {
public:
    List();
    void SetRowHeight(float h) noexcept { rowHeight = h; }
    void SetItemFontSize(float px) noexcept { itemFontPx = px; }
    void SetItemBold(bool b) noexcept;
    /** When set, row buttons use solid surfaces (no semi-transparent theme fill or shadow). */
    void SetOpaqueRows(bool v) noexcept;
    /** When set, scroll viewport fill uses alpha 1.0 (dropdown popups on the late layer). */
    void SetOpaqueViewport(bool v) noexcept { opaqueViewport = v; }
    void SetItems(Array<Utf8String> lines);
    [[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex; }
    /** Updates selection highlight without invoking <c>SetOnSelectionChanged</c>. */
    void SetSelectedIndex(int i) noexcept;
    void SetOnSelectionChanged(std::function<void(int)> fn) { onSelect = Spark::MoveTemp(fn); }

    /** Pixels of content scrolled past the viewport top (larger → rows move up). */
    void SetScrollY(float y) noexcept { scrollY = y; }
    [[nodiscard]] float GetScrollY() const noexcept { return scrollY; }
    void ScrollToTop() noexcept;

    /**
     * When false, the list never clips or scrolls: row stack uses the full width, grows <c>bounds.height</c> to fit
     * all rows. Use a parent <c>ScrollPanel</c> when the launcher needs both full row paint and wheel scrolling.
     */
    void SetVerticalScrollingEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IsVerticalScrollingEnabled() const noexcept { return verticalScrollingEnabled; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

    /** Applied by the GUI layer when the cursor is over this list; uses current <c>maxScroll</c> from layout. */
    void ApplyScrollWheelDelta(float deltaY) noexcept;

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;

private:
    void Rebuild();
    void SyncScrollLayout() noexcept;
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    [[nodiscard]] bool RowIntersectsViewport(const Rect& rowBounds) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void ScrollSelectedIndexIntoView() noexcept;
    void SnapScrollToRowGrid(float scaledRowH) noexcept;

    float rowHeight = 30.0F;
    float itemFontPx = 24.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    Array<Utf8String> items{};
    int selectedIndex = -1;
    std::function<void(int)> onSelect{};

    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;
    Rect trackRect{};
    Rect thumbRect{};
    Rect arrangeRect{};
    static constexpr float kTrackW = 12.0F;
    bool verticalScrollingEnabled = true;
    bool opaqueViewport = false;
};

}  // namespace Gui
}  // namespace Spark
