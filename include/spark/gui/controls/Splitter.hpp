#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

enum class SplitterOrientation {
    Horizontal,
    Vertical,
};

/**
 * Draggable split between exactly two child widgets (creation order: first / second pane).
 */
class Splitter final : public Widget {
public:
    void SetOrientation(SplitterOrientation o) noexcept { orientation = o; }
    void SetSplit(float t) noexcept {
        if (t < 0.08F) {
            split = 0.08F;
        } else if (t > 0.92F) {
            split = 0.92F;
        } else {
            split = t;
        }
    }
    [[nodiscard]] float GetSplit() const noexcept { return split; }

    /** Half-width (horizontal) or half-height (vertical) of the draggable gutter in pixels. */
    void SetGutterHalfWidth(float halfPx) noexcept { gutterHalf = halfPx < 2.0F ? 2.0F : halfPx; }

    /**
     * Called when the user drags the gutter (<c>committed == false</c>) and when the drag ends
     * (<c>committed == true</c>). <c>split</c> is the fraction along the inner area (see <c>GetSplit</c>).
     */
    void SetOnSplitChanged(std::function<void(float split, bool committed)> fn) {
        onSplitChanged = Spark::MoveTemp(fn);
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    [[nodiscard]] bool HitGutter(float x, float y) const noexcept;

    SplitterOrientation orientation = SplitterOrientation::Horizontal;
    float split = 0.5F;
    bool dragging = false;
    float gutterHalf = 3.0F;
    std::function<void(float split, bool committed)> onSplitChanged{};
};

}  // namespace Gui
}  // namespace Spark
