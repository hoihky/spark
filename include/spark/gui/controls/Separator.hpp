#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Thin rule for toolbars / inspectors (non-interactive). */
class Separator final : public Widget {
public:
    enum class Orientation {
        Horizontal,
        Vertical,
    };

    Separator();
    void SetOrientation(Orientation o) noexcept { orientation = o; }
    void SetThickness(float t) noexcept { thickness = t; }
    void SetMargin(float m) noexcept { margin = m; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    Orientation orientation = Orientation::Horizontal;
    float thickness = 1.0F;
    float margin = 6.0F;
};

}  // namespace Spark::Gui
