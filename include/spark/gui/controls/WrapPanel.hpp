#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

/**
 * Flows children left-to-right; when the next slot would exceed the panel width, starts a new row.
 * Each child is assigned a fixed slot size (configure with <c>SetSlotSize</c>).
 */
class WrapPanel final : public Widget {
public:
    void SetSlotSize(float w, float h) noexcept {
        slotW = w;
        slotH = h;
    }
    void SetGaps(float horizontal, float vertical) noexcept {
        hGap = horizontal;
        vGap = vertical;
    }

    void Arrange(const Rect& r) override;

private:
    float slotW = 120.0F;
    float slotH = 32.0F;
    float hGap = 8.0F;
    float vGap = 8.0F;
};

}  // namespace Spark::Gui
