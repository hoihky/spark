#include "spark/gui/controls/Separator.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

Separator::Separator() {
    hitTest = false;
}

void Separator::Arrange(const Rect& r) {
    bounds = r;
}

void Separator::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const Vector3 lineRgb = th.borderRgb;
    const float a = 0.55F;
    if (orientation == Orientation::Horizontal) {
        const float y = bounds.y + bounds.height * 0.5F - thickness * 0.5F;
        const float x0 = bounds.x + margin;
        const float w = (std::max)(0.0F, bounds.width - 2.0F * margin);
        ctx.FillRect(x0, y, w, thickness, lineRgb, a);
    } else {
        const float x = bounds.x + bounds.width * 0.5F - thickness * 0.5F;
        const float y0 = bounds.y + margin;
        const float h = (std::max)(0.0F, bounds.height - 2.0F * margin);
        ctx.FillRect(x, y0, thickness, h, lineRgb, a);
    }
}

}  // namespace Spark::Gui
