#include "spark/gui/controls/CheckBox.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

CheckBox::CheckBox() = default;

void CheckBox::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float drawFont = m.Scaled(labelFontPx);
    const float box = (std::max)(m.Scaled(20.0F), drawFont * 0.92F);
    const float boxTop = bounds.y + (std::max)(m.Scaled(2.0F), (bounds.height - box) * 0.5F);
    ctx.FillRectGradientVertical(bounds.x, boxTop, box, box, th.checkFrameTop, th.checkFrameBottom, 0.92F);
    ctx.StrokeRect(bounds.x, boxTop, box, box, 1.0F, th.borderRgb, 0.7F);
    if (checked) {
        const float inset = box * 0.18F;
        ctx.FillRectGradientVertical(bounds.x + inset, boxTop + inset, box - 2.0F * inset, box - 2.0F * inset,
                th.checkFillTop, th.checkFillBottom, 0.98F);
        ctx.StrokeRect(bounds.x + inset, boxTop + inset, box - 2.0F * inset, box - 2.0F * inset, 1.0F,
                th.checkInnerStrokeRgb, 0.55F);
    }
    const float textY = bounds.y + (std::max)(m.Scaled(2.0F), (bounds.height - drawFont) * 0.5F);
    ctx.DrawText(bounds.x + box + m.Scaled(12.0F), textY, drawFont, caption, th.labelPrimary, 1.0F);
}

void CheckBox::NotifyClick(const GuiFrameInput&, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    checked = !checked;
    if (onChanged) {
        onChanged(checked);
    }
}

}  // namespace Spark::Gui
