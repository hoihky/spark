#include "spark/core/Utf8String.hpp"
#include "spark/ui/spark/controls/SparkTextBox.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/core/UiTheme.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>

namespace {

Spark::Utf8String SubstringBytes(const Spark::Utf8String& src, const std::size_t begin, const std::size_t end) {
    Spark::Utf8String out{};
    const char* c = src.CStr();
    const std::size_t limit = src.ByteLength();
    for (std::size_t i = begin; i < end && i < limit; ++i) {
        const char ch[2]{c[i], '\0'};
        out.AppendUtf8(ch);
    }
    return out;
}

std::size_t Utf8ByteLengthOfCodepoint(const std::uint32_t codepoint) noexcept {
    if (codepoint <= 0x7FU) {
        return 1U;
    }
    if (codepoint <= 0x7FFU) {
        return 2U;
    }
    if (codepoint <= 0xFFFFU) {
        return 3U;
    }
    return 4U;
}

bool IsPrintableCodepoint(const std::uint32_t codepoint) noexcept {
    return codepoint >= 32U && codepoint != 127U;
}

float ClampWidth(const Spark::Ui::UiMeasureConstraints& constraints, const float desired) {
    float width = desired;
    if (constraints.maxWidth > 0.0F) {
        width = width < constraints.maxWidth ? width : constraints.maxWidth;
    }
    return width < constraints.minWidth ? constraints.minWidth : width;
}

float ClampHeight(const Spark::Ui::UiMeasureConstraints& constraints, const float desired) {
    float height = desired;
    if (constraints.maxHeight > 0.0F) {
        height = height < constraints.maxHeight ? height : constraints.maxHeight;
    }
    return height < constraints.minHeight ? constraints.minHeight : height;
}

}  // namespace

namespace Spark::Ui {

SparkTextBox::SparkTextBox(const TextFieldDesc& desc)
    : UiElementBase(desc.id), label(desc.label), committed(desc.text), draft(desc.text) {
    SetEnabled(desc.enabled);
    caretByteIndex = draft.ByteLength();
}

void SparkTextBox::SetText(Utf8String textIn) {
    if (focused) {
        return;
    }
    committed = MoveTemp(textIn);
    draft = committed;
    caretByteIndex = draft.ByteLength();
}

void SparkTextBox::OnFocusGained() {
    focused = true;
    draft = committed;
    caretByteIndex = draft.ByteLength();
}

void SparkTextBox::OnFocusLost() {
    if (focused) {
        CommitDraft();
    }
    focused = false;
}

void SparkTextBox::CommitDraft() {
    if (draft != committed) {
        committed = draft;
        onCommit.Invoke();
    }
}

void SparkTextBox::AppendCodepoint(const std::uint32_t codepoint) {
    if (!IsPrintableCodepoint(codepoint)) {
        return;
    }
    Utf8String next = SubstringBytes(draft, 0, caretByteIndex);
    next.AppendCodepoint(codepoint);
    next.AppendUtf8(SubstringBytes(draft, caretByteIndex, draft.ByteLength()).CStr());
    draft = MoveTemp(next);
    caretByteIndex += Utf8ByteLengthOfCodepoint(codepoint);
}

void SparkTextBox::DeleteBeforeCaret() {
    if (caretByteIndex == 0U || draft.IsEmpty()) {
        return;
    }
    std::size_t prev = caretByteIndex - 1U;
    while (prev > 0U && (static_cast<unsigned char>(draft.CStr()[prev]) & 0xC0U) == 0x80U) {
        --prev;
    }
    const Utf8String suffix = SubstringBytes(draft, caretByteIndex, draft.ByteLength());
    draft = SubstringBytes(draft, 0, prev);
    draft.AppendUtf8(suffix.CStr());
    caretByteIndex = prev;
}

void SparkTextBox::DeleteAfterCaret() {
    if (caretByteIndex >= draft.ByteLength()) {
        return;
    }
    std::size_t next = caretByteIndex + 1U;
    while (next < draft.ByteLength() && (static_cast<unsigned char>(draft.CStr()[next]) & 0xC0U) == 0x80U) {
        ++next;
    }
    const Utf8String suffix = SubstringBytes(draft, next, draft.ByteLength());
    draft = SubstringBytes(draft, 0, caretByteIndex);
    draft.AppendUtf8(suffix.CStr());
}

void SparkTextBox::MoveCaretLeft() {
    if (caretByteIndex == 0U) {
        return;
    }
    std::size_t prev = caretByteIndex - 1U;
    while (prev > 0U && (static_cast<unsigned char>(draft.CStr()[prev]) & 0xC0U) == 0x80U) {
        --prev;
    }
    caretByteIndex = prev;
}

void SparkTextBox::MoveCaretRight() {
    if (caretByteIndex >= draft.ByteLength()) {
        return;
    }
    std::size_t next = caretByteIndex + 1U;
    while (next < draft.ByteLength() && (static_cast<unsigned char>(draft.CStr()[next]) & 0xC0U) == 0x80U) {
        ++next;
    }
    caretByteIndex = next;
}

void SparkTextBox::MoveCaretHome() {
    caretByteIndex = 0U;
}

void SparkTextBox::MoveCaretEnd() {
    caretByteIndex = draft.ByteLength();
}

void SparkTextBox::ProcessKeyInput(IInput& input) {
    if (!IsEnabled()) {
        return;
    }

    Array<std::uint32_t> typed{};
    input.DrainTypedCodepoints(typed);
    for (std::size_t i = 0; i < typed.GetSize(); ++i) {
        AppendCodepoint(typed[i]);
    }

    const bool ctrl = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
    if (ctrl && input.IsKeyPressedThisFrame(GLFW_KEY_V)) {
        Utf8String clip{};
        if (input.TryGetClipboardUtf8(clip)) {
            for (const char* p = clip.CStr(); *p != '\0'; ++p) {
                AppendCodepoint(static_cast<unsigned char>(*p));
            }
        }
    }

    if (input.IsKeyPressedThisFrame(GLFW_KEY_BACKSPACE)) {
        DeleteBeforeCaret();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_DELETE)) {
        DeleteAfterCaret();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
        MoveCaretLeft();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
        MoveCaretRight();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        MoveCaretHome();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_END)) {
        MoveCaretEnd();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_ENTER) || input.IsKeyPressedThisFrame(GLFW_KEY_KP_ENTER)) {
        CommitDraft();
    }
}

void SparkTextBox::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampWidth(constraints, metrics.Scaled(200.0F));
    const float labelH = label.IsEmpty() ? 0.0F : metrics.FontSmall() + metrics.Scaled(2.0F);
    outDesired.height = ClampHeight(constraints, labelH + metrics.FormRowHeight());
}

void SparkTextBox::DoPaint(IUiRenderer& renderer) {
    const UiTheme& theme = renderer.GetTheme();
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const Rect b = GetBounds();
    float fieldY = b.y;
    if (!label.IsEmpty()) {
        renderer.DrawText(b.x, b.y, b.width, label, theme.labelMuted, 1.0F, metrics.FontSmall(), false);
        fieldY += metrics.FontSmall() + metrics.Scaled(4.0F);
    }
    const float fieldH = metrics.FormRowHeight() - (fieldY - b.y);
    const Vector3& border = focused ? theme.textBoxBorderFocus : theme.textBoxBorderIdle;
    renderer.FillRectGradientVertical(
            b.x, fieldY, b.width, fieldH, theme.textBoxFillTop, theme.textBoxFillBottom, theme.textBoxFillAlpha);
    renderer.StrokeRect(b.x, fieldY, b.width, fieldH, 1.0F, border, focused ? 0.95F : 0.75F);
    const float pad = metrics.Scaled(8.0F);
    renderer.DrawText(
            b.x + pad,
            fieldY + (fieldH - metrics.FontControl()) * 0.5F,
            b.width - pad * 2.0F,
            draft,
            theme.labelPrimary,
            1.0F,
            metrics.FontControl(),
            false);
    if (focused) {
        const float caretX = b.x + pad + metrics.Scaled(2.0F);
        renderer.FillRect(caretX, fieldY + metrics.Scaled(6.0F), 1.5F, fieldH - metrics.Scaled(12.0F), border, 0.95F);
    }
}

}  // namespace Spark::Ui
