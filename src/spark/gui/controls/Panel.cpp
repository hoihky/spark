#include "spark/gui/controls/Panel.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

namespace Spark::Gui {

void Panel::Arrange(const Rect& r) {
    bounds = r;
    const float pad = GetActiveGuiLayoutMetrics().Scaled(padding);
    const Rect inner = r.Inset(pad);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i]) {
            children[i]->Arrange(inner);
        }
    }
}

void Panel::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    if (dropShadow) {
        ctx.FillDropShadow(
                bounds.x, bounds.y, bounds.width, bounds.height, m.Scaled(5.0F), m.Scaled(6.0F), th.shadowRgb, 1.0F);
    }
    if (!backgroundEnabled) {
        PaintChildren(ctx);
        return;
    }
    if (dropdownListThemeBound) {
        /** Dropdown popups must be fully opaque so underlying widgets do not bleed through. */
        ctx.FillRectGradientVertical(bounds.x,
                bounds.y,
                bounds.width,
                bounds.height,
                th.dropdownPanelTop,
                th.dropdownPanelBottom,
                1.0F);
    } else if (useGradient) {
        ctx.FillRectGradientVertical(
                bounds.x, bounds.y, bounds.width, bounds.height, bg, bg2, bgAlpha);
    } else {
        ctx.FillRect(bounds.x, bounds.y, bounds.width, bounds.height, bg, bgAlpha);
    }
    if (chrome) {
        ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, th.borderRgb, 0.48F);
    }
    PaintChildren(ctx);
}

}  // namespace Spark::Gui
