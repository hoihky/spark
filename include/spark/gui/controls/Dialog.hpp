#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

class Label;
class Panel;
class GuiPaintContext;

/**
 * Modal-style frame: dimmer over full viewport + titled panel. Add content widgets to ContentPanel().
 */
class Dialog final : public Widget {
public:
    Dialog();
    [[nodiscard]] Panel* ContentPanel() noexcept { return body; }
    void SetTitle(Utf8String t);
    void SetTitleFontSize(float px);
    void SetBodySize(float w, float h) noexcept {
        bodyW = w;
        bodyH = h;
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    Panel* dimmer = nullptr;
    Panel* body = nullptr;
    Label* titleLabel = nullptr;
    float bodyW = 420.0F;
    float bodyH = 280.0F;
};

}  // namespace Spark::Gui
