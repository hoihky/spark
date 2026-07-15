#pragma once

#include <cstdint>

namespace Spark::Gui {

/** Screen-space axis-aligned rectangle (framebuffer pixels, Y downward). */
struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    [[nodiscard]] bool Contains(float px, float py) const noexcept {
        return px >= x && py >= y && px < x + width && py < y + height;
    }

    [[nodiscard]] Rect Inset(float pad) const noexcept {
        return {x + pad, y + pad, width - 2.0F * pad, height - 2.0F * pad};
    }
};

/** Normalized pointer sample for one frame (from IInput + cursor position). */
struct GuiFrameInput {
    float mouseX = 0.0F;
    float mouseY = 0.0F;
    bool leftDown = false;
    bool leftPressedThisFrame = false;
    bool leftReleasedThisFrame = false;
    bool rightDown = false;
    bool rightPressedThisFrame = false;
    bool ctrlDown = false;
    bool shiftDown = false;
    bool altDown = false;
    /** From <c>IInput::GetScrollDeltaY</c> for this frame (GUI may route to lists, etc.). */
    float scrollDeltaY = 0.0F;
};

/** How text behaves when it exceeds the horizontal or vertical layout bounds. */
enum class TextOverflow : std::uint8_t {
    /** Draw past the layout rect (legacy single-line behavior). */
    Visible = 0,
    /** Scissor-clip content to the rect. */
    Clip,
    /** Truncate with an ellipsis (…) on the last visible line. */
    Ellipsis,
};

/** Whether long text may break across multiple lines. */
enum class TextWrap : std::uint8_t {
    NoWrap = 0,
    WordWrap,
};

/** Per-widget text layout policy used by <c>GuiPaintContext::DrawTextInRect</c>. */
struct TextLayout {
    TextOverflow overflow = TextOverflow::Ellipsis;
    TextWrap wrap = TextWrap::NoWrap;
    /** When wrapping: max lines to emit; 0 derives from rect height. */
    int maxLines = 0;
};

}  // namespace Spark::Gui
