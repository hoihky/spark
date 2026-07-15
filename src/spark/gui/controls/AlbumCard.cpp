#include "spark/gui/controls/AlbumCard.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

void AlbumCard::Arrange(const Rect& r) {
    bounds = r;
    constexpr float pad = 14.0F;
    constexpr float rail = 4.0F;
    const float innerX = r.x + pad + (accentRail ? rail + 6.0F : 0.0F);
    const float innerW = std::max(40.0F, r.width - 2.0F * pad - (accentRail ? rail + 6.0F : 0.0F));
    const float artH = std::max(72.0F, r.height * 0.48F);
    const float y0 = r.y + pad;
    artRect = {innerX, y0, innerW, artH};
    const float textY = y0 + artH + 10.0F;
    titleRect = {innerX, textY, innerW, titleFontPx * 1.25F};
    subtitleRect = {innerX, textY + titleFontPx * 1.15F, innerW, subtitleFontPx * 1.2F};
}

void AlbumCard::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.FillDropShadow(bounds.x, bounds.y, bounds.width, bounds.height, 4.0F, 8.0F, th.shadowRgb, 0.85F);
    ctx.FillRectGradientVertical(
            bounds.x, bounds.y, bounds.width, bounds.height, th.albumCardTop, th.albumCardBottom, th.albumCardAlpha);
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, th.borderRgb, 0.45F);
    if (accentRail) {
        ctx.FillRect(bounds.x + 6.0F, bounds.y + 10.0F, 4.0F, bounds.height - 20.0F, th.albumAccentBarRgb, th.albumAccentBarAlpha);
    }
    ctx.FillRectGradientVertical(artRect.x,
            artRect.y,
            artRect.width,
            artRect.height,
            th.albumArtPlaceholderTop,
            th.albumArtPlaceholderBottom,
            1.0F);
    ctx.StrokeRect(artRect.x, artRect.y, artRect.width, artRect.height, 1.0F, th.borderRgb, 0.35F);
    if (!title.IsEmpty()) {
        ctx.DrawText(titleRect.x, titleRect.y, titleFontPx, title, th.labelPrimary, 1.0F, true);
    }
    if (!subtitle.IsEmpty()) {
        ctx.DrawText(subtitleRect.x, subtitleRect.y, subtitleFontPx, subtitle, th.labelMuted, 0.95F, false);
    }
}

}  // namespace Spark::Gui
