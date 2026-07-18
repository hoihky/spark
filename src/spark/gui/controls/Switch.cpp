#include "spark/gui/controls/Switch.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>

namespace Spark::Gui {

Switch::Switch() = default;

void Switch::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& t = ctx.GetTheme();
    const float w = bounds.width;
    const float h = bounds.height * 0.52F;
    const float y = bounds.y + (bounds.height - h) * 0.5F;
    const Vector3& offA = on ? t.switchTrackOnTop : t.switchTrackOffTop;
    const Vector3& offB = on ? t.switchTrackOnBottom : t.switchTrackOffBottom;
    ctx.FillRectGradientVertical(bounds.x, y, w, h, offA, offB, 0.94F);
    ctx.StrokeRect(bounds.x, y, w, h, 1.0F, t.borderRgb, 0.55F);
    const float knob = h * 0.86F;
    const float travel = std::max(0.0F, w - knob - 6.0F);
    const float kx = bounds.x + 3.0F + (on ? travel : 0.0F);
    const float ky = y + (h - knob) * 0.5F;
    ctx.FillDropShadow(kx, ky, knob, knob, 1.0F, 1.5F, t.shadowRgb, 0.45F);
    ctx.FillRectGradientVertical(kx, ky, knob, knob, t.switchKnobTop, t.switchKnobBottom, 0.98F);
    ctx.StrokeRect(kx, ky, knob, knob, 1.0F, t.borderRgb, 0.4F);
}

void Switch::NotifyClick(const GuiFrameInput&, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    on = !on;
    if (onChanged) {
        onChanged(on);
    }
}

}  // namespace Spark::Gui
