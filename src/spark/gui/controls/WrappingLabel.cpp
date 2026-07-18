#include "spark/gui/controls/WrappingLabel.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

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
    const GuiTheme& th = ctx.GetTheme();
    const Vector3 drawColor =
            tone == LabelTone::Muted ? th.labelMuted : (tone == LabelTone::Custom ? color : th.labelPrimary);
    Rect textRect = bounds;
    if (textRect.width <= 4.0F) {
        textRect.width = 64.0F;
    }
    ctx.DrawTextInRect(textRect, drawFont, text, drawColor, 1.0F, bold, layout);
}

}  // namespace Spark::Gui
