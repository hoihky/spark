#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Non-interactive label with optional word wrap and overflow handling. */
class Label final : public Widget {
public:
    Label();
    void SetText(Utf8String t) { text = Spark::MoveTemp(t); }
    [[nodiscard]] const Utf8String& GetText() const noexcept { return text; }
    void SetFontSize(float px) noexcept { fontPx = px; }
    void SetTextColor(const Vector3& c) noexcept { color = c; }
    void SetBold(bool b) noexcept { bold = b; }
    void SetTextLayout(TextLayout value) noexcept { layout = value; }
    void SetTextOverflow(TextOverflow overflow) noexcept { layout.overflow = overflow; }
    void SetTextWrap(TextWrap wrap) noexcept { layout.wrap = wrap; }
    void SetMaxLines(int lines) noexcept { layout.maxLines = lines; }

    void Paint(GuiPaintContext& ctx) const override;

private:
    Utf8String text{};
    float fontPx = 22.0F;
    Vector3 color{0.10F, 0.22F, 0.13F};
    bool bold = false;
    TextLayout layout{};
};

}  // namespace Spark::Gui
