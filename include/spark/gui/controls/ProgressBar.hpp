#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Filled bar for 0..1 completion ratio (non-interactive). */
class ProgressBar final : public Widget {
public:
    ProgressBar();
    void SetValue01(float v) noexcept {
        if (v < 0.0F) {
            value01 = 0.0F;
        } else if (v > 1.0F) {
            value01 = 1.0F;
        } else {
            value01 = v;
        }
    }
    [[nodiscard]] float GetValue01() const noexcept { return value01; }
    void SetPreferSkinChrome(bool v) noexcept { preferSkinChrome = v; }

    void Paint(GuiPaintContext& ctx) const override;

private:
    float value01 = 0.35F;
    bool preferSkinChrome = false;
};

}  // namespace Spark::Gui
