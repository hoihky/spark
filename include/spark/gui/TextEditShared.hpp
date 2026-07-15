#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/engine/IInput.hpp"

namespace Spark::Gui {

/** UTF-8 codepoint count. */
[[nodiscard]] std::size_t CountCodepoints(const Utf8String& s) noexcept;

/** Byte index of codepoint boundary at logical index (0..count). */
[[nodiscard]] std::size_t CodepointOffsetToByteIndex(const Utf8String& s, std::size_t codepointIndex) noexcept;

[[nodiscard]] Utf8String DropLastCodepoint(const Utf8String& s);
[[nodiscard]] Utf8String EraseCodepointBeforeCaret(Utf8String& s, std::size_t& caretCodepoints);
[[nodiscard]] Utf8String EraseCodepointAtCaret(Utf8String& s, std::size_t& caretCodepoints);

void InsertUtf8AtCaret(Utf8String& s, std::size_t& caretCodepoints, const char* utf8);
void InsertUtf8AtCaret(Utf8String& s, std::size_t& caretCodepoints, std::size_t& selectionAnchor, const char* utf8);

[[nodiscard]] bool IsShiftDown(const IInput& in) noexcept;
[[nodiscard]] bool IsCtrlDown(const IInput& in) noexcept;

/** GLFW key scan for printable ASCII when focused (not exhaustive). Returns true if a key was consumed. */
bool ProcessPrintableKeyScan(IInput& input, Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor);

bool ProcessTextEditNavigationKeys(
        IInput& input, std::size_t& caretCodepoints, std::size_t& selectionAnchor, std::size_t codepointCount);
bool ProcessTextEditClipboardKeys(
        IInput& input, Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor);

/** Removes the selected codepoint range when <c>caret != selectionAnchor</c>. */
void DeleteSelectionIfAny(Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor) noexcept;

[[nodiscard]] Utf8String Utf8SubstringCodepoints(const Utf8String& s, std::size_t startCp, std::size_t endCp) noexcept;

[[nodiscard]] bool HasTextSelection(std::size_t caretCodepoints, std::size_t selectionAnchor) noexcept;

}  // namespace Spark::Gui
