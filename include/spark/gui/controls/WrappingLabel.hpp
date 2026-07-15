#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Multi-line label; text wraps to widget width with ellipsis on overflow. */
class WrappingLabel final : public Widget {
public:
    WrappingLabel();
    void SetText(Utf8String t) { text = Spark::MoveTemp(t); }
    [[nodiscard]] const Utf8String& GetText() const noexcept { return text; }
    void SetFontSize(float px) noexcept { fontPx = px; }
    void SetTextColor(const Vector3& c) noexcept { color = c; }
    void SetBold(bool b) noexcept { bold = b; }
    void SetTextLayout(TextLayout layout) noexcept { layout_ = layout; }
    void SetMaxLines(int lines) noexcept { layout_.maxLines = lines; }

    void Paint(GuiPaintContext& ctx) const override;

private:
    Utf8String text{};
    float fontPx = 22.0F;
    Vector3 color{0.10F, 0.22F, 0.13F};
    bool bold = false;
    TextLayout layout_{TextOverflow::Ellipsis, TextWrap::WordWrap, 0};
};

}  // namespace Spark::Gui
