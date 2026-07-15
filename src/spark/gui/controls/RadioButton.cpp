#include "spark/gui/controls/RadioButton.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/RadioGroup.hpp"

#include <algorithm>

namespace Spark::Gui {

RadioButton::RadioButton() = default;

RadioButton::~RadioButton() {
    if (group != nullptr) {
        group->Unregister(this);
        group = nullptr;
    }
}

void RadioButton::SetGroup(RadioGroup* g) noexcept {
    if (group == g) {
        return;
    }
    if (group != nullptr) {
        group->Unregister(this);
    }
    group = g;
    if (group != nullptr) {
        group->Register(this);
    }
}

void RadioButton::ApplyGroupSelection(bool selected) noexcept {
    if (checked == selected) {
        return;
    }
    checked = selected;
    if (onChanged) {
        onChanged(selected);
    }
}

void RadioButton::SetChecked(bool v) {
    if (group != nullptr) {
        if (v) {
            group->Select(this);
        } else if (group->GetSelected() == this) {
            group->Select(nullptr);
        }
        return;
    }
    if (checked != v) {
        checked = v;
        if (onChanged) {
            onChanged(v);
        }
    }
}

void RadioButton::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float box = (std::max)(20.0F, labelFontPx * 0.92F);
    const float boxTop = bounds.y + (std::max)(2.0F, (bounds.height - box) * 0.5F);
    const float cx = bounds.x + box * 0.5F;
    const float cy = boxTop + box * 0.5F;
    const float rad = box * 0.5F - 1.5F;

    ctx.FillRoundRectSolid(bounds.x, boxTop, box, box, rad, th.checkFrameTop, 0.88F);
    ctx.StrokeRoundRect(bounds.x, boxTop, box, box, rad, 1.0F, th.borderRgb, 0.65F);
    if (ctx.IsHot(this) && !ctx.IsActive(this)) {
        ctx.StrokeRoundRect(bounds.x + 1.0F, boxTop + 1.0F, box - 2.0F, box - 2.0F, rad - 1.0F, 1.0F, th.controlHotTop, 0.35F);
    }
    if (checked) {
        const float inset = rad * 0.42F;
        ctx.FillRoundRectSolid(cx - rad + inset, cy - rad + inset, (rad - inset) * 2.0F, (rad - inset) * 2.0F,
                (rad - inset) * 0.45F, th.checkFillTop, 0.98F);
    }
    const float textY = bounds.y + (std::max)(2.0F, (bounds.height - labelFontPx) * 0.5F);
    ctx.DrawText(bounds.x + box + 12.0F, textY, labelFontPx, caption, th.labelPrimary, 1.0F);
}

void RadioButton::NotifyClick(const GuiFrameInput&, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    if (group != nullptr) {
        group->Select(this);
        return;
    }
    checked = !checked;
    if (onChanged) {
        onChanged(checked);
    }
}

}  // namespace Spark::Gui
