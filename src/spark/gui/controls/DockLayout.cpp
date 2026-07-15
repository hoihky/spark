#include "spark/gui/controls/DockLayout.hpp"

#include <algorithm>

namespace Spark::Gui {

void DockLayout::Arrange(const Rect& r) {
    bounds = r;
    const auto& ch = GetChildren();
    if (ch.IsEmpty()) {
        return;
    }
    if (ch.GetSize() == 1U) {
        if (ch[0]) {
            ch[0]->Arrange(r);
        }
        return;
    }
    if (orientation == DockOrientation::Horizontal) {
        const float w0 = r.width * firstFrac;
        const float w1 = std::max(0.0F, r.width - w0);
        if (ch[0]) {
            ch[0]->Arrange({r.x, r.y, w0, r.height});
        }
        if (ch[1]) {
            ch[1]->Arrange({r.x + w0, r.y, w1, r.height});
        }
    } else {
        const float h0 = r.height * firstFrac;
        const float h1 = std::max(0.0F, r.height - h0);
        if (ch[0]) {
            ch[0]->Arrange({r.x, r.y, r.width, h0});
        }
        if (ch[1]) {
            ch[1]->Arrange({r.x, r.y + h0, r.width, h1});
        }
    }
}

}  // namespace Spark::Gui
