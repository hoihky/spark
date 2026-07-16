#include "spark/gui/controls/WrappingLabel.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"

namespace Spark::Gui {

WrappingLabel::WrappingLabel() {
    SetHitTest(false);
}

void WrappingLabel::Paint(GuiPaintContext& ctx) const {
    if (!visible || text.IsEmpty()) {
        return;
    }
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float drawFont = m.Scaled(fontPx);
    Rect textRect = bounds;
    if (textRect.width <= 4.0F) {
        textRect.width = 64.0F;
    }
    ctx.DrawTextInRect(textRect, drawFont, text, color, 1.0F, bold, layout);
}

}  // namespace Spark::Gui
