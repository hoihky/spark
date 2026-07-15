#pragma once

#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;

/** On/off pill switch; click toggles state. */
class Switch final : public Widget {
public:
    Switch();
    [[nodiscard]] bool IsOn() const noexcept { return on; }
    void SetOn(bool v) noexcept { on = v; }
    void SetOnChanged(std::function<void(bool)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    bool on = false;
    std::function<void(bool)> onChanged{};
};

}  // namespace Spark::Gui
