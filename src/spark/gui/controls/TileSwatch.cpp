#include "spark/gui/controls/TileSwatch.hpp"

#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <cmath>

namespace Spark::Gui {

void TileSwatch::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float cw = checkerPx;
    const int nx = static_cast<int>(std::ceil(static_cast<double>(bounds.width / cw)));
    const int ny = static_cast<int>(std::ceil(static_cast<double>(bounds.height / cw)));
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix < nx; ++ix) {
            const bool a = ((ix + iy) & 1) == 0;
            const float x = bounds.x + static_cast<float>(ix) * cw;
            const float y = bounds.y + static_cast<float>(iy) * cw;
            const float rw = std::min(cw, bounds.x + bounds.width - x);
            const float rh = std::min(cw, bounds.y + bounds.height - y);
            if (rw > 0.5F && rh > 0.5F) {
                const Vector3 c = a ? th.controlIdleTop : th.controlIdleBottom;
                ctx.FillRect(x, y, rw, rh, c, 0.55F);
            }
        }
    }
    if (textureLayer >= 0) {
        const float cx = bounds.x + bounds.width * 0.5F;
        const float cy = bounds.y + bounds.height * 0.5F;
        const float sw = std::max(4.0F, bounds.width - 16.0F);
        const float sh = std::max(4.0F, bounds.height - 24.0F);
        const Matrix4 model =
                Matrix4::Translation({cx, cy, 0.0F}) * Matrix4::Scale({sw, sh, 1.0F});
        ctx.EmitSprite(model, textureLayer, Vector4{0.0F, 0.0F, 1.0F, 1.0F}, Vector4{1.0F, 1.0F, 1.0F, 1.0F});
    }
    ctx.StrokeRect(bounds.x, bounds.y, bounds.width, bounds.height, 1.0F, th.borderRgb, 0.65F);
    if (!caption.IsEmpty()) {
        const float ty = bounds.y + bounds.height - 18.0F;
        ctx.DrawText(bounds.x + 6.0F, ty, 14.0F, caption, th.labelPrimary, 0.95F);
    }
}

}  // namespace Spark::Gui
