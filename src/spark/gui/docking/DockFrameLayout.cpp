#include "spark/gui/docking/DockFrameLayout.hpp"

#include <algorithm>

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/docking/DockSidePane.hpp"

namespace Spark::Gui {

void DockFrameLayout::SetLeftPane(UniquePtr<DockSidePane> pane) {
    leftPane_ = pane.Get();
    AddChild(MoveTemp(pane));
}

void DockFrameLayout::SetCenter(UniquePtr<Widget> center) {
    center_ = center.Get();
    if (center_) {
        center_->SetHitTest(false);
    }
    AddChild(MoveTemp(center));
}

void DockFrameLayout::SetRightPane(UniquePtr<DockSidePane> pane) {
    rightPane_ = pane.Get();
    AddChild(MoveTemp(pane));
}

void DockFrameLayout::Arrange(const Rect& r) {
    bounds = r;
    const float leftW = leftPane_ != nullptr ? leftPane_->GetOccupiedWidth() : 0.0F;
    const float rightW = rightPane_ != nullptr ? rightPane_->GetOccupiedWidth() : 0.0F;
    const float centerW = std::max(0.0F, r.width - leftW - rightW);
    float x = r.x;
    if (leftPane_) {
        leftPane_->Arrange({x, r.y, leftW, r.height});
        x += leftW;
    }
    if (center_) {
        const Rect centerRect{x, r.y, centerW, r.height};
        center_->Arrange(centerRect);
        centerBounds_ = centerRect;
        x += centerW;
    } else {
        centerBounds_ = {0.0F, 0.0F, 0.0F, 0.0F};
    }
    if (rightPane_) {
        rightPane_->Arrange({x, r.y, rightW, r.height});
    }
}

void DockFrameLayout::Paint(GuiPaintContext& ctx) const {
    if (leftPane_) {
        leftPane_->Paint(ctx);
    }
    if (center_) {
        center_->Paint(ctx);
    }
    if (rightPane_) {
        rightPane_->Paint(ctx);
    }
}

Widget* DockFrameLayout::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (rightPane_) {
        if (Widget* hit = rightPane_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (center_) {
        if (Widget* hit = center_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (leftPane_) {
        if (Widget* hit = leftPane_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

}  // namespace Spark::Gui
