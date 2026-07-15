#include "spark/gui/controls/Label.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"

#include <algorithm>

namespace Spark::Gui {

Label::Label() {
    SetHitTest(false);
}

void Label::Paint(GuiPaintContext& ctx) const {
    if (!visible || text.IsEmpty()) {
        return;
    }
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float drawFont = m.Scaled(fontPx);
    if (bounds.width <= 0.0F || bounds.height <= 0.0F) {
        ctx.DrawText(bounds.x, bounds.y, drawFont, text, color, 1.0F, bold);
        return;
    }
    Rect textRect = bounds;
    if (layout_.wrap == TextWrap::NoWrap) {
        textRect.height = std::min(bounds.height, drawFont);
    }
    ctx.DrawTextInRect(textRect, drawFont, text, color, 1.0F, bold, layout_);
}

}  // namespace Spark::Gui
