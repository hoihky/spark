#include "spark/gui/controls/Image.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiSkin.hpp"

#include <algorithm>

namespace Spark::Gui {

void Image::Arrange(const Rect& r) {
    if (!usePreferredSize) {
        bounds = r;
        return;
    }
    const float w = std::min(r.width, preferredW);
    const float h = std::min(r.height, preferredH);
    bounds = {r.x + (r.width - w) * 0.5F, r.y + (r.height - h) * 0.5F, w, h};
}

void Image::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    GuiSpriteSlice drawSlice = spriteSlice;
    if (useSkinElement) {
        const GuiSkin* skin = ctx.GetSkin();
        if (skin == nullptr || !skin->TryGetSlice(skinElement, drawSlice)) {
            return;
        }
    }
    if (!drawSlice.IsValid()) {
        return;
    }
    float drawW = bounds.width;
    float drawH = bounds.height;
    float drawX = bounds.x;
    float drawY = bounds.y;
    if (preserveAspect && drawSlice.texture) {
        const float texW = static_cast<float>(std::max(1U, drawSlice.texture->GetWidth()));
        const float texH = static_cast<float>(std::max(1U, drawSlice.texture->GetHeight()));
        const float srcW = (drawSlice.uvRect.z - drawSlice.uvRect.x) * texW;
        const float srcH = (drawSlice.uvRect.w - drawSlice.uvRect.y) * texH;
        if (srcW > 0.5F && srcH > 0.5F) {
            const float aspect = srcW / srcH;
            if (drawW / std::max(drawH, 1.0e-4F) > aspect) {
                const float newW = drawH * aspect;
                drawX += (drawW - newW) * 0.5F;
                drawW = newW;
            } else {
                const float newH = drawW / aspect;
                drawY += (drawH - newH) * 0.5F;
                drawH = newH;
            }
        }
    }
    if (drawSlice.nineSlice.IsEmpty()) {
        ctx.DrawSpriteSlice(drawSlice, drawX, drawY, drawW, drawH, tint, alpha);
    } else {
        ctx.DrawNineSlice(drawSlice, bounds.x, bounds.y, bounds.width, bounds.height, tint, alpha);
    }
}

}  // namespace Spark::Gui
