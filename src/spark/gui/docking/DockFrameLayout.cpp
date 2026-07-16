#include "spark/gui/docking/DockFrameLayout.hpp"

#include <algorithm>

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/docking/DockSidePane.hpp"

namespace Spark::Gui {

void DockFrameLayout::SetLeftPane(UniquePtr<DockSidePane> pane) {
    leftPane = pane.Get();
    AddChild(MoveTemp(pane));
}

void DockFrameLayout::SetCenter(UniquePtr<Widget> widget) {
    center = widget.Get();
    if (center) {
        center->SetHitTest(false);
    }
    AddChild(MoveTemp(widget));
}

void DockFrameLayout::SetRightPane(UniquePtr<DockSidePane> pane) {
    rightPane = pane.Get();
    AddChild(MoveTemp(pane));
}

void DockFrameLayout::Arrange(const Rect& r) {
    bounds = r;
    const float leftW = leftPane != nullptr ? leftPane->GetOccupiedWidth() : 0.0F;
    const float rightW = rightPane != nullptr ? rightPane->GetOccupiedWidth() : 0.0F;
    const float centerW = std::max(0.0F, r.width - leftW - rightW);
    float x = r.x;
    if (leftPane) {
        leftPane->Arrange({x, r.y, leftW, r.height});
        x += leftW;
    }
    if (center) {
        const Rect centerRect{x, r.y, centerW, r.height};
        center->Arrange(centerRect);
        centerBounds = centerRect;
        x += centerW;
    } else {
        centerBounds = {0.0F, 0.0F, 0.0F, 0.0F};
    }
    if (rightPane) {
        rightPane->Arrange({x, r.y, rightW, r.height});
    }
}

void DockFrameLayout::Paint(GuiPaintContext& ctx) const {
    if (leftPane) {
        leftPane->Paint(ctx);
    }
    if (center) {
        center->Paint(ctx);
    }
    if (rightPane) {
        rightPane->Paint(ctx);
    }
}

Widget* DockFrameLayout::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (rightPane) {
        if (Widget* hit = rightPane->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (center) {
        if (Widget* hit = center->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (leftPane) {
        if (Widget* hit = leftPane->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

}  // namespace Spark::Gui
