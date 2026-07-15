#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;

/** Toggle with label; click flips checked state. */
class CheckBox final : public Widget {
public:
    CheckBox();
    void SetLabel(Utf8String t) { caption = Spark::MoveTemp(t); }
    [[nodiscard]] bool IsChecked() const noexcept { return checked; }
    void SetChecked(bool v) noexcept { checked = v; }
    void SetOnChanged(std::function<void(bool)> fn) { onChanged = Spark::MoveTemp(fn); }
    void SetFontSize(float px) noexcept { labelFontPx = px; }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    Utf8String caption{Utf8String("Check")};
    float labelFontPx = 22.0F;
    bool checked = false;
    std::function<void(bool)> onChanged{};
};

}  // namespace Spark::Gui
