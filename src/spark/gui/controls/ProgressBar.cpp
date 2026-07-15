#include "spark/gui/controls/ProgressBar.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

ProgressBar::ProgressBar() {
    SetHitTest(false);
}

void ProgressBar::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.FillRect(bounds.x, bounds.y, bounds.width, bounds.height, th.progressTrackRgb, 0.55F);
    const float fillW = std::max(0.0F, bounds.width * value01);
    ctx.FillRectGradientHorizontal(bounds.x, bounds.y, fillW, bounds.height, th.progressFillTop,
            th.progressFillBottom, 0.92F);
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, th.borderRgb, 0.45F);
}

}  // namespace Spark::Gui
