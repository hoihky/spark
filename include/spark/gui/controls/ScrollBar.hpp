#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;

enum class ScrollBarAxis {
    Vertical,
    Horizontal,
};

/** Standalone 0..1 scroll indicator with draggable thumb. */
class ScrollBar final : public Widget {
public:
    ScrollBar();
    void SetAxis(ScrollBarAxis a) noexcept { axis = a; }
    void SetValue01(float v) noexcept {
        if (v < 0.0F) {
            value01 = 0.0F;
        } else if (v > 1.0F) {
            value01 = 1.0F;
        } else {
            value01 = v;
        }
    }
    [[nodiscard]] float GetValue01() const noexcept { return value01; }
    void SetOnChanged(std::function<void(float)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    void SyncFromPointer(float x, float y);

    ScrollBarAxis axis = ScrollBarAxis::Vertical;
    float value01 = 0.0F;
    bool dragging = false;
    float grabAlong = 0.0F;
    std::function<void(float)> onChanged{};

    float thumbStart = 0.0F;
    float thumbLen = 0.0F;
    float trackLen = 0.0F;
};

}  // namespace Spark::Gui
