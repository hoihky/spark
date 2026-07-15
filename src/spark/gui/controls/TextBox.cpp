#include "spark/gui/controls/TextBox.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/TextEditShared.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace Spark::Gui {

TextBox::TextBox() = default;

void TextBox::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float rr = th.textBoxCornerRadius;
    ctx.FillDropShadow(
            bounds.x, bounds.y, bounds.width, bounds.height, 3.0F, 3.5F, th.shadowRgb, 0.55F);
    ctx.FillRoundRectGradientVertical(bounds.x, bounds.y, bounds.width, bounds.height, rr, th.textBoxFillTop,
            th.textBoxFillBottom, th.textBoxFillAlpha);
    const Vector3 border = ctx.IsFocused(this) ? th.textBoxBorderFocus : th.textBoxBorderIdle;
    ctx.StrokeRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, rr, 1.0F, border,
            ctx.IsFocused(this) ? 0.9F : 0.55F);
    const Utf8String& draw = value.IsEmpty() ? placeholder : value;
    const Vector3 tc = value.IsEmpty() ? th.labelMuted : th.labelPrimary;
    const float textY = bounds.y + std::max(4.0F, (bounds.height - textPx) * 0.5F);
    const float padX = ctx.GetLayoutMetrics().Padding();
    if (ctx.IsFocused(this) && !value.IsEmpty()) {
        const std::size_t lo = std::min(caretCodepoints, selectionAnchor);
        const std::size_t hi = std::max(caretCodepoints, selectionAnchor);
        const Utf8String before = Utf8SubstringCodepoints(draw, 0, lo);
        const Utf8String mid = Utf8SubstringCodepoints(draw, lo, hi);
        const Utf8String after = Utf8SubstringCodepoints(draw, hi, CountCodepoints(draw));
        const float beforeW = ctx.MeasureUtf8Width(before, textPx);
        const float midW = ctx.MeasureUtf8Width(mid, textPx);
        if (HasTextSelection(caretCodepoints, selectionAnchor) && midW > 0.0F) {
            ctx.FillRect(bounds.x + padX + beforeW, textY - 1.0F, midW, textPx + 2.0F, th.controlAccentTop, 0.45F);
        }
        ctx.DrawText(bounds.x + padX, textY, textPx, before, tc, 1.0F);
        const float cx = bounds.x + padX + beforeW + midW;
        ctx.FillRect(cx, textY, 2.0F, textPx * 0.85F, th.labelPrimary, 0.92F);
        ctx.DrawText(cx + 3.0F, textY, textPx, after, tc, 1.0F);
    } else if (ctx.IsFocused(this)) {
        const float cx = bounds.x + padX;
        ctx.FillRect(cx, textY, 2.0F, textPx * 0.85F, th.labelPrimary, 0.92F);
        ctx.DrawText(cx + 3.0F, textY, textPx, draw, tc, 1.0F);
    } else {
        ctx.DrawText(bounds.x + padX, textY, textPx, draw, tc, 1.0F);
    }
}

void TextBox::ProcessKeyInput(IInput& input) {
    if (!enabled) {
        return;
    }
    const std::size_t cpCount = CountCodepoints(value);
    if (ProcessTextEditClipboardKeys(input, value, caretCodepoints, selectionAnchor)) {
        if (onChanged) {
            onChanged(value);
        }
        return;
    }
    if (ProcessTextEditNavigationKeys(input, caretCodepoints, selectionAnchor, cpCount)) {
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_BACKSPACE)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            DeleteSelectionIfAny(value, caretCodepoints, selectionAnchor);
            if (onChanged) {
                onChanged(value);
            }
        } else if (caretCodepoints > 0) {
            (void)EraseCodepointBeforeCaret(value, caretCodepoints);
            selectionAnchor = caretCodepoints;
            if (onChanged) {
                onChanged(value);
            }
        }
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_DELETE)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            DeleteSelectionIfAny(value, caretCodepoints, selectionAnchor);
            if (onChanged) {
                onChanged(value);
            }
        } else if (caretCodepoints < cpCount) {
            (void)EraseCodepointAtCaret(value, caretCodepoints);
            selectionAnchor = caretCodepoints;
            if (onChanged) {
                onChanged(value);
            }
        }
        return;
    }
    if (ProcessPrintableKeyScan(input, value, caretCodepoints, selectionAnchor)) {
        if (onChanged) {
            onChanged(value);
        }
    }
}

}  // namespace Spark::Gui
