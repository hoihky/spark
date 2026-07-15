#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class GuiPaintContext;
class RadioGroup;

/**
 * Single-choice toggle (circle + label). Wire <c>SetGroup</c> to a <c>RadioGroup</c> for tool modes / layers.
 * Without a group, behaves like a binary toggle and fires <c>SetOnChanged</c>.
 */
class RadioButton final : public Widget {
public:
    RadioButton();
    ~RadioButton() override;

    void SetLabel(Utf8String t) { caption = Spark::MoveTemp(t); }
    void SetFontSize(float px) noexcept { labelFontPx = px; }
    /** When null, click toggles checked locally (no mutual exclusion). */
    void SetGroup(RadioGroup* g) noexcept;
    [[nodiscard]] RadioGroup* GetGroup() const noexcept { return group; }
    [[nodiscard]] bool IsChecked() const noexcept { return checked; }
    /** Sets state; with a group, selecting false clears selection if this was selected. */
    void SetChecked(bool v);
    void SetOnChanged(std::function<void(bool)> fn) { onChanged = Spark::MoveTemp(fn); }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    friend class RadioGroup;
    void ApplyGroupSelection(bool selected) noexcept;

    Utf8String caption{Utf8String("Option")};
    float labelFontPx = 22.0F;
    bool checked = false;
    RadioGroup* group = nullptr;
    std::function<void(bool)> onChanged{};
};

}  // namespace Spark::Gui
