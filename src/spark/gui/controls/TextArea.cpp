#include "spark/gui/controls/TextArea.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/TextEditShared.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace Spark::Gui {

TextArea::TextArea() {
    placeholder = Utf8String("Enter text…");
}

void TextArea::Arrange(const Rect& r) {
    bounds = r;
}

void TextArea::NotifyChanged() {
    if (onChanged) {
        onChanged(value);
    }
}

void TextArea::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const float rr = th.textBoxCornerRadius;
    ctx.FillDropShadow(bounds.x, bounds.y, bounds.width, bounds.height, 3.0F, 3.5F, th.shadowRgb, 0.55F);
    ctx.FillRoundRectGradientVertical(bounds.x, bounds.y, bounds.width, bounds.height, rr, th.textBoxFillTop,
            th.textBoxFillBottom, th.textBoxFillAlpha);
    const Vector3 border = ctx.IsFocused(this) ? th.textBoxBorderFocus : th.textBoxBorderIdle;
    ctx.StrokeRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, rr, 1.0F, border,
            ctx.IsFocused(this) ? 0.9F : 0.55F);

    const Utf8String& draw = value.IsEmpty() ? placeholder : value;
    const Vector3 tc = value.IsEmpty() ? th.labelMuted : th.labelPrimary;
    const float padX = ctx.GetLayoutMetrics().Padding();
    const float padY = ctx.GetLayoutMetrics().ControlGap() + 2.0F;
    float y = bounds.y + padY - scrollY;
    Utf8String line;
    std::uint32_t cp = 0;
    std::size_t cpIndex = 0;
    for (auto it = draw.Iterator(); it.NextCodepoint(cp);) {
        if (cp == '\n' || cp == 0x0A) {
            if (!line.IsEmpty() || cpIndex == caretCodepoints) {
                ctx.DrawText(bounds.x + padX, y, textPx, line, tc, 1.0F);
            }
            if (cpIndex == caretCodepoints && ctx.IsFocused(this)) {
                const float cx = bounds.x + padX + 2.0F;
                ctx.FillRect(cx, y, 2.0F, lineHeightPx - 2.0F, th.labelPrimary, 0.9F);
            }
            line.Clear();
            y += lineHeightPx;
            ++cpIndex;
            continue;
        }
        line.AppendCodepoint(cp);
        if (cpIndex == caretCodepoints && ctx.IsFocused(this)) {
            const float cx = bounds.x + padX + ctx.MeasureUtf8Width(line, textPx);
            ctx.FillRect(cx, y, 2.0F, lineHeightPx - 2.0F, th.labelPrimary, 0.9F);
        }
        ++cpIndex;
    }
    if (!line.IsEmpty() || (caretCodepoints == cpIndex && ctx.IsFocused(this))) {
        ctx.DrawText(bounds.x + padX, y, textPx, line, tc, 1.0F);
        if (caretCodepoints == cpIndex && ctx.IsFocused(this)) {
            const float cx = bounds.x + padX + ctx.MeasureUtf8Width(line, textPx);
            ctx.FillRect(cx, y, 2.0F, lineHeightPx - 2.0F, th.labelPrimary, 0.9F);
        }
    }
}

void TextArea::ProcessKeyInput(IInput& input) {
    if (!enabled) {
        return;
    }
    const std::size_t cpCount = CountCodepoints(value);
    if (ProcessTextEditClipboardKeys(input, value, caretCodepoints, selectionAnchor)) {
        NotifyChanged();
        return;
    }
    if (ProcessTextEditNavigationKeys(input, caretCodepoints, selectionAnchor, cpCount)) {
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_ENTER) && !IsShiftDown(input)) {
        InsertUtf8AtCaret(value, caretCodepoints, selectionAnchor, "\n");
        NotifyChanged();
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_BACKSPACE)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            DeleteSelectionIfAny(value, caretCodepoints, selectionAnchor);
            NotifyChanged();
        } else if (caretCodepoints > 0) {
            (void)EraseCodepointBeforeCaret(value, caretCodepoints);
            selectionAnchor = caretCodepoints;
            NotifyChanged();
        }
        return;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_DELETE)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            DeleteSelectionIfAny(value, caretCodepoints, selectionAnchor);
            NotifyChanged();
        } else if (caretCodepoints < cpCount) {
            (void)EraseCodepointAtCaret(value, caretCodepoints);
            selectionAnchor = caretCodepoints;
            NotifyChanged();
        }
        return;
    }
    if (ProcessPrintableKeyScan(input, value, caretCodepoints, selectionAnchor)) {
        NotifyChanged();
    }
}

}  // namespace Spark::Gui
