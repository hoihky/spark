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
    void SetTone(LabelTone value) noexcept { tone = value; }
    void SetTextColor(const Vector3& c) noexcept {
        tone = LabelTone::Custom;
        color = c;
    }
    void SetBold(bool b) noexcept { bold = b; }
    void SetTextLayout(TextLayout value) noexcept { layout = value; }
    void SetMaxLines(int lines) noexcept { layout.maxLines = lines; }

    void Paint(GuiPaintContext& ctx) const override;

private:
    Utf8String text{};
    float fontPx = 22.0F;
    LabelTone tone = LabelTone::Primary;
    Vector3 color{};
    bool bold = false;
    TextLayout layout{TextOverflow::Ellipsis, TextWrap::WordWrap, 0};
};

}  // namespace Spark::Gui
