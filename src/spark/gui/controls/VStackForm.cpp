#include "spark/gui/controls/VStackForm.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"

#include <algorithm>

namespace Spark::Gui {

void VStackForm::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = children.GetSize();
    if (n == 0U) {
        return;
    }
    const bool custom = rowHeights.GetSize() == n;
    const float defaultRowH = GetActiveGuiLayoutMetrics().FormRowHeight();
    float y = r.y;
    for (std::size_t i = 0; i < n; ++i) {
        if (!children[i]) {
            continue;
        }
        if (!children[i]->IsVisible()) {
            children[i]->Arrange({r.x, y, r.width, 0.0F});
            continue;
        }
        const float h = custom ? std::max(4.0F, rowHeights[i]) : defaultRowH;
        children[i]->Arrange({r.x, y, r.width, h});
        y += h + vGap;
    }
}

void VStackForm::Paint(GuiPaintContext& ctx) const {
    PaintChildren(ctx);
}

}  // namespace Spark::Gui
