#include "spark/gui/controls/Button.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

void Button::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& t = ctx.GetTheme();
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float drawFont = m.Scaled(fontPx);
    const float drawIcon = m.Scaled(iconGlyphPx);
    const float padX = m.Scaled(10.0F);
    const float padInner = m.Scaled(8.0F);
    const float shadowOff = m.Scaled(2.5F);
    const float shadowBlur = m.Scaled(3.0F);
    const float cornerR = m.Scaled(t.controlCornerRadius);
    Vector3 top = t.controlIdleTop;
    Vector3 bot = t.controlIdleBottom;
    Vector3 textRgb = t.labelPrimary;
    if (accentSelected) {
        top = t.controlAccentTop;
        bot = t.controlAccentBottom;
        textRgb = t.labelOnAccent;
    } else {
        if (ctx.IsHot(this)) {
            top = t.controlHotTop;
            bot = t.controlHotBottom;
        }
        if (ctx.IsActive(this)) {
            top = t.controlActiveTop;
            bot = t.controlActiveBottom;
        }
    }
    const float fillAlpha = opaqueSurface ? 1.0F : t.controlFillAlpha;
    const float strokeAlpha = opaqueSurface ? 1.0F : t.controlStrokeAlpha;
    if (!opaqueSurface) {
        ctx.FillDropShadow(
                bounds.x, bounds.y, bounds.width, bounds.height, shadowOff, shadowBlur, t.shadowRgb, 0.72F);
    }
    ctx.FillRoundRectGradientVertical(
            bounds.x, bounds.y, bounds.width, bounds.height, cornerR, top, bot, fillAlpha);
    ctx.StrokeRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, cornerR, 1.0F, t.borderRgb, strokeAlpha);
    const float textY = bounds.y + std::max(m.Scaled(2.0F), (bounds.height - drawFont) * 0.5F);
    float textX = bounds.x + padX;
    float maxTextW = bounds.width - padX * 2.0F;
    if (!iconGlyph.IsEmpty()) {
        const float isz = std::min(drawIcon, std::max(m.Scaled(10.0F), bounds.height - m.Scaled(6.0F)));
        const float iy = bounds.y + std::max(m.Scaled(2.0F), (bounds.height - isz) * 0.5F);
        ctx.DrawText(bounds.x + padInner, iy, isz, iconGlyph, textRgb, 1.0F, labelBold);
        textX = bounds.x + padInner + isz + m.Scaled(6.0F);
        maxTextW = bounds.x + bounds.width - padX - textX;
    } else if (iconTexture) {
        const float isz = std::min(drawIcon, std::max(m.Scaled(10.0F), bounds.height - m.Scaled(6.0F)));
        const float iy = bounds.y + std::max(m.Scaled(2.0F), (bounds.height - isz) * 0.5F);
        const float ir = std::min(m.Scaled(6.0F), isz * 0.28F);
        ctx.FillRoundRectSolid(bounds.x + padInner, iy, isz, isz, ir, t.controlHotTop, 0.78F);
        ctx.StrokeRoundRect(bounds.x + padInner, iy, isz, isz, ir, 1.0F, t.borderRgb, strokeAlpha * 0.85F);
        textX = bounds.x + padInner + isz + m.Scaled(6.0F);
        maxTextW = bounds.x + bounds.width - padX - textX;
    }
    if (!label.IsEmpty() && maxTextW > 2.0F) {
        const Utf8String drawn = ctx.EllipsizeUtf8(label, drawFont, maxTextW);
        ctx.DrawText(textX, textY, drawFont, drawn, textRgb, 1.0F, labelBold);
    }
}

void Button::NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (!enabled) {
        return;
    }
    if (onClickWithFrame) {
        onClickWithFrame(in, canvas);
        return;
    }
    if (onClick) {
        onClick();
    }
}

}  // namespace Spark::Gui
