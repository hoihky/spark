#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Vertical stack with scroll offset and an integrated scrollbar strip on the right. */
class ScrollPanel final : public Widget {
public:
    ScrollPanel();
    void SetRowHeight(float h) noexcept { rowHeight = h; }
    void SetVerticalGap(float g) noexcept { vGap = g; }
    /**
     * When this array's length matches the number of children, each child i is given height
     * <c>heights[i]</c> instead of the uniform <c>rowHeight</c>. Pass an empty array to use uniform rows.
     */
    void SetPerChildRowHeights(Array<float> heights) noexcept { perChildHeights = Spark::MoveTemp(heights); }
    /** Pixels of virtual content scrolled past the viewport top (thumb down → larger value → content moves up). */
    void SetScrollY(float y) noexcept { scrollY = y; }
    [[nodiscard]] float GetScrollY() const noexcept { return scrollY; }
    void ScrollToTop() noexcept;

    /** When set, the scrolled content area uses this fill instead of the global GUI theme (e.g. opaque editor cards). */
    void SetViewportFillGradient(const Vector3& topRgb, const Vector3& bottomRgb, float alpha) noexcept {
        useCustomViewportFill = true;
        customViewportTop = topRgb;
        customViewportBottom = bottomRgb;
        customViewportAlpha = alpha;
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

    /** Mouse wheel (same convention as <c>List</c>): positive delta scrolls content up. */
    void ApplyScrollWheelDelta(float deltaY) noexcept;

private:
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void SyncScrollLayout() noexcept;

    float rowHeight = 28.0F;
    Array<float> perChildHeights{};
    float vGap = 4.0F;
    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;

    Rect trackRect{};
    Rect thumbRect{};
    Rect arrangeRect{};
    static constexpr float kTrackW = 12.0F;

    bool useCustomViewportFill = false;
    Vector3 customViewportTop{};
    Vector3 customViewportBottom{};
    float customViewportAlpha = 1.0F;
};

}  // namespace Spark::Gui
