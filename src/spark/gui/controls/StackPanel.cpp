#include "spark/gui/controls/StackPanel.hpp"

#include <algorithm>

namespace Spark::Gui {

void StackPanel::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = children.GetSize();
    if (n == 0) {
        return;
    }
    const float totalSpacing = spacing * static_cast<float>(n > 0 ? n - 1 : 0);
    if (orientation == StackOrientation::Vertical) {
        const float slotH = std::max(0.0F, (r.height - totalSpacing) / static_cast<float>(n));
        float y = r.y;
        for (std::size_t i = 0; i < n; ++i) {
            if (children[i]) {
                children[i]->Arrange({r.x, y, r.width, slotH});
            }
            y += slotH + spacing;
        }
    } else {
        const float slotW = std::max(0.0F, (r.width - totalSpacing) / static_cast<float>(n));
        float x = r.x;
        for (std::size_t i = 0; i < n; ++i) {
            if (children[i]) {
                children[i]->Arrange({x, r.y, slotW, r.height});
            }
            x += slotW + spacing;
        }
    }
}

}  // namespace Spark::Gui
