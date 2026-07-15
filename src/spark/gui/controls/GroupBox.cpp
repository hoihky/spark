#include "spark/gui/controls/GroupBox.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

GroupBox::GroupBox() = default;

void GroupBox::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = children.GetSize();
    const float innerY = r.y + titleBarH + padding;
    const float innerH = (std::max)(0.0F, r.height - titleBarH - padding * 2.0F);
    const float innerW = (std::max)(0.0F, r.width - padding * 2.0F);
    const float innerX = r.x + padding;
    if (n == 0) {
        return;
    }
    const float totalSp = childSpacing * static_cast<float>(n > 0 ? n - 1 : 0);
    const float slotH = (std::max)(0.0F, (innerH - totalSp) / static_cast<float>(n));
    float y = innerY;
    for (std::size_t i = 0; i < n; ++i) {
        if (children[i]) {
            children[i]->Arrange({innerX, y, innerW, slotH});
        }
        y += slotH + childSpacing;
    }
}

void GroupBox::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float rr = th.controlCornerRadius;
    ctx.FillRoundRectGradientVertical(
            bounds.x, bounds.y, bounds.width, bounds.height, rr, th.tabBodyTop, th.tabBodyBottom, th.tabBodyAlpha);
    ctx.StrokeRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, rr, 1.0F, th.borderRgb, 0.55F);

    ctx.FillRectGradientVertical(bounds.x + 1.0F, bounds.y + 1.0F, bounds.width - 2.0F, titleBarH - 1.0F,
            th.controlIdleTop, th.controlIdleBottom, 0.88F);
    const float textY = bounds.y + (std::max)(3.0F, (titleBarH - titleFontPx) * 0.5F);
    ctx.DrawText(bounds.x + 12.0F, textY, titleFontPx, title, th.labelPrimary, 1.0F);

    PaintChildren(ctx);
}

}  // namespace Spark::Gui
