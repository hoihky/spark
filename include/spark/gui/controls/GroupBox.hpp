#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Titled frame: draws a header strip and lays out **all children** in a vertical stack below it
 * (equal-height rows). Use for inspector sections and map properties.
 */
class GroupBox final : public Widget {
public:
    GroupBox();
    void SetTitle(Utf8String t) { title = Spark::MoveTemp(t); }
    void SetTitleFontSize(float px) noexcept { titleFontPx = px; }
    void SetTitleBarHeight(float h) noexcept { titleBarH = h; }
    void SetPadding(float p) noexcept { padding = p; }
    void SetChildSpacing(float s) noexcept { childSpacing = s; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    Utf8String title{Utf8String("Group")};
    float titleFontPx = 20.0F;
    float titleBarH = 30.0F;
    float padding = 10.0F;
    float childSpacing = 6.0F;
};

}  // namespace Spark::Gui
