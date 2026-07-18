#include "spark/gui/controls/Label.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

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
    const GuiTheme& th = ctx.GetTheme();
    const Vector3 drawColor =
            tone == LabelTone::Muted ? th.labelMuted : (tone == LabelTone::Custom ? color : th.labelPrimary);
    if (bounds.width <= 0.0F || bounds.height <= 0.0F) {
        ctx.DrawText(bounds.x, bounds.y, drawFont, text, drawColor, 1.0F, bold);
        return;
    }
    Rect textRect = bounds;
    if (layout.wrap == TextWrap::NoWrap) {
        textRect.height = std::min(bounds.height, drawFont);
    }
    ctx.DrawTextInRect(textRect, drawFont, text, drawColor, 1.0F, bold, layout);
}

}  // namespace Spark::Gui
