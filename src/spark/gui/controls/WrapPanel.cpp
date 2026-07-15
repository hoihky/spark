#include "spark/gui/controls/WrapPanel.hpp"

#include <algorithm>

namespace Spark::Gui {

void WrapPanel::Arrange(const Rect& r) {
    bounds = r;
    const float cellW = std::max(8.0F, slotW);
    const float cellH = std::max(8.0F, slotH);
    float x = r.x;
    float y = r.y;
    float rowH = 0.0F;
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (x + cellW > r.x + r.width + 0.5F && x > r.x + 0.5F) {
            x = r.x;
            y += rowH + vGap;
            rowH = 0.0F;
        }
        if (children[i]) {
            children[i]->Arrange({x, y, cellW, cellH});
        }
        rowH = std::max(rowH, cellH);
        x += cellW + hGap;
    }
}

}  // namespace Spark::Gui
