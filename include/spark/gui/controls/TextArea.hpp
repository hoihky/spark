#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/TextEditShared.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class IInput;

namespace Gui {

class GuiPaintContext;

/** Multiline editable text (GLFW keys when focused). Supports caret, navigation, and clipboard. */
class TextArea final : public Widget {
public:
    TextArea();
    void SetPlaceholder(Utf8String t) { placeholder = Spark::MoveTemp(t); }
    void SetTextValue(Utf8String t) {
        value = Spark::MoveTemp(t);
        caretCodepoints = CountCodepoints(value);
        selectionAnchor = caretCodepoints;
    }
    [[nodiscard]] const Utf8String& GetTextValue() const noexcept { return value; }
    void SetOnChanged(std::function<void(const Utf8String&)> fn) { onChanged = Spark::MoveTemp(fn); }
    void SetFontSize(float px) noexcept { textPx = px; }
    void SetLineHeight(float px) noexcept { lineHeightPx = px; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    void ProcessKeyInput(IInput& input) override;
    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }

private:
    void NotifyChanged();

    Utf8String value{};
    Utf8String placeholder{Utf8String("…")};
    float textPx = 18.0F;
    float lineHeightPx = 22.0F;
    std::size_t caretCodepoints = 0;
    std::size_t selectionAnchor = 0;
    float scrollY = 0.0F;
    std::function<void(const Utf8String&)> onChanged{};
};

}  // namespace Gui
}  // namespace Spark
