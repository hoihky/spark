#include "spark/gui/controls/ProgressBar.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiSkin.hpp"
#include "spark/gui/GuiSkinElement.hpp"
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

    if (preferSkinChrome) {
        const GuiSkin* skin = ctx.GetSkin();
        GuiSpriteSlice trackSlice{};
        GuiSpriteSlice fillSlice{};
        if (skin != nullptr && skin->TryGetSlice(GuiSkinElement::ProgressBarTrack, trackSlice) && trackSlice.IsValid() &&
                skin->TryGetSlice(GuiSkinElement::ProgressBarFill, fillSlice) && fillSlice.IsValid()) {
            ctx.DrawNineSlice(trackSlice, bounds.x, bounds.y, bounds.width, bounds.height);
            const float fillW = std::max(4.0F, bounds.width * value01);
            if (fillW > 4.0F) {
                ctx.DrawNineSlice(fillSlice, bounds.x, bounds.y, fillW, bounds.height);
            }
            return;
        }
    }

    ctx.FillRect(bounds.x, bounds.y, bounds.width, bounds.height, th.progressTrackRgb, 0.55F);
    const float fillW = std::max(0.0F, bounds.width * value01);
    ctx.FillRectGradientHorizontal(
            bounds.x, bounds.y, fillW, bounds.height, th.progressFillTop, th.progressFillBottom, 0.92F);
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, th.borderRgb, 0.45F);
}

}  // namespace Spark::Gui
