#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

/**
 * Vertical form layout: each child gets an explicit height from <c>SetRowHeights</c> (length must match child
 * count). Invisible children are skipped and consume no vertical space. Prefer this over <c>StackPanel</c> for
 * inspector rows — <c>StackPanel</c> divides height equally among children.
 */
class VStackForm final : public Widget {
public:
    void SetVerticalGap(float g) noexcept { vGap = g; }
    void SetRowHeights(Array<float> heights) noexcept { rowHeights = Spark::MoveTemp(heights); }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    float vGap = 6.0F;
    Array<float> rowHeights{};
};

}  // namespace Spark::Gui
