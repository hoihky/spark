#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

/**
 * Places child 0 in a fixed-width left strip and child 1 in the remainder. The right pane defaults to
 * <c>SetHitTest(false)</c> so pointer input can reach game/world picking behind the UI.
 * A draggable gutter on the right edge of the sidebar resizes the strip.
 */
class EditorSidebarLayout final : public Widget {
public:
    void SetSidebarWidth(float widthPx) noexcept { sidebarWidthPx = widthPx; }
    [[nodiscard]] float GetSidebarWidth() const noexcept { return sidebarWidthPx; }

    void SetOnSidebarWidthChanged(std::function<void(float widthPx, bool committed)> fn) {
        onSidebarWidthChanged = Spark::MoveTemp(fn);
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    [[nodiscard]] bool HitGutter(float x, float y) const noexcept;

    float sidebarWidthPx = 300.0F;
    float gutterHalf = 3.0F;
    Rect gutterRect{};
    bool dragging = false;
    std::function<void(float widthPx, bool committed)> onSidebarWidthChanged{};
};

}  // namespace Gui
}  // namespace Spark
