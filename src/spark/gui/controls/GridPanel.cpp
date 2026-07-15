#include "spark/gui/controls/GridPanel.hpp"

#include "spark/gui/GuiPaintContext.hpp"

#include <algorithm>

namespace Spark::Gui {

void GridPanel::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = children.GetSize();
    if (n == 0) {
        return;
    }
    const std::uint32_t cols = columns;
    const std::size_t rows = (n + static_cast<std::size_t>(cols) - 1U) / static_cast<std::size_t>(cols);
    const float colsF = static_cast<float>(cols);
    const float rowsF = static_cast<float>(rows);
    const float totalHGap = hSpacing * (std::max)(0.0F, colsF - 1.0F);
    const float totalVGap = vSpacing * (std::max)(0.0F, rowsF - 1.0F);
    const float cellW = (std::max)(0.0F, (r.width - totalHGap) / colsF);
    const float cellH = (std::max)(0.0F, (r.height - totalVGap) / rowsF);
    for (std::size_t i = 0; i < n; ++i) {
        if (!children[i]) {
            continue;
        }
        const std::size_t col = i % static_cast<std::size_t>(cols);
        const std::size_t row = i / static_cast<std::size_t>(cols);
        const float x = r.x + static_cast<float>(col) * (cellW + hSpacing);
        const float y = r.y + static_cast<float>(row) * (cellH + vSpacing);
        children[i]->Arrange({x, y, cellW, cellH});
    }
}

void GridPanel::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    PaintChildren(ctx);
}

}  // namespace Spark::Gui
