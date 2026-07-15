#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class IInput;

namespace Gui {

class GuiPaintContext;

/** Single-line editable field (GLFW key scan when focused). */
class TextBox final : public Widget {
public:
    TextBox();
    void SetPlaceholder(Utf8String t) { placeholder = Spark::MoveTemp(t); }
    void SetTextValue(Utf8String t) { value = Spark::MoveTemp(t); }
    [[nodiscard]] const Utf8String& GetTextValue() const noexcept { return value; }
    void SetOnChanged(std::function<void(const Utf8String&)> fn) { onChanged = Spark::MoveTemp(fn); }
    void SetFontSize(float px) noexcept { textPx = px; }

    void Paint(GuiPaintContext& ctx) const override;
    void ProcessKeyInput(IInput& input) override;
    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }

private:
    Utf8String value{};
    Utf8String placeholder{Utf8String("Type here...")};
    float textPx = 20.0F;
    std::size_t caretCodepoints = 0;
    std::size_t selectionAnchor = 0;
    std::function<void(const Utf8String&)> onChanged{};
};

}  // namespace Gui
}  // namespace Spark
