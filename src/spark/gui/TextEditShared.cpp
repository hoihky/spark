#include "spark/gui/TextEditShared.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>

namespace Spark::Gui {

namespace {

void CollectCodepoints(const Utf8String& s, Array<std::uint32_t>& out) {
    out.Clear();
    std::uint32_t cp = 0;
    for (auto it = s.Iterator(); it.NextCodepoint(cp);) {
        out.PushBack(cp);
    }
}

Utf8String FromCodepoints(const Array<std::uint32_t>& cps) {
    Utf8String out;
    for (std::size_t i = 0; i < cps.GetSize(); ++i) {
        out.AppendCodepoint(cps[i]);
    }
    return out;
}

void AppendAsciiChar(Utf8String& s, const char c) {
    const char buf[2] = {c, '\0'};
    s.AppendUtf8(buf);
}

}  // namespace

std::size_t CountCodepoints(const Utf8String& s) noexcept {
    std::size_t n = 0;
    std::uint32_t cp = 0;
    for (auto it = s.Iterator(); it.NextCodepoint(cp);) {
        ++n;
    }
    return n;
}

std::size_t CodepointOffsetToByteIndex(const Utf8String& s, const std::size_t codepointIndex) noexcept {
    if (codepointIndex == 0) {
        return 0;
    }
    const char* p = s.CStr();
    if (p == nullptr) {
        return 0;
    }
    std::size_t cp = 0;
    std::size_t byteIdx = 0;
    while (p[byteIdx] != '\0' && cp < codepointIndex) {
        const auto u0 = static_cast<unsigned char>(p[byteIdx]);
        std::size_t adv = 1;
        if (u0 >= 0x80) {
            if ((u0 >> 5) == 0x6) {
                adv = 2;
            } else if ((u0 >> 4) == 0xE) {
                adv = 3;
            } else if ((u0 >> 3) == 0x1E) {
                adv = 4;
            }
        }
        byteIdx += adv;
        ++cp;
    }
    return byteIdx;
}

Utf8String DropLastCodepoint(const Utf8String& s) {
    Array<std::uint32_t> cps;
    CollectCodepoints(s, cps);
    if (cps.IsEmpty()) {
        return {};
    }
    cps.PopBack();
    return FromCodepoints(cps);
}

Utf8String EraseCodepointBeforeCaret(Utf8String& s, std::size_t& caretCodepoints) {
    if (caretCodepoints == 0) {
        return s;
    }
    Array<std::uint32_t> cps;
    CollectCodepoints(s, cps);
    if (cps.IsEmpty()) {
        caretCodepoints = 0;
        s.Clear();
        return s;
    }
    const std::size_t idx = caretCodepoints - 1;
    if (idx < cps.GetSize()) {
        for (std::size_t j = idx; j + 1 < cps.GetSize(); ++j) {
            cps[j] = cps[j + 1];
        }
        cps.PopBack();
    }
    --caretCodepoints;
    s = FromCodepoints(cps);
    return s;
}

Utf8String EraseCodepointAtCaret(Utf8String& s, std::size_t& caretCodepoints) {
    Array<std::uint32_t> cps;
    CollectCodepoints(s, cps);
    if (caretCodepoints >= cps.GetSize()) {
        return s;
    }
    for (std::size_t j = caretCodepoints; j + 1 < cps.GetSize(); ++j) {
        cps[j] = cps[j + 1];
    }
    cps.PopBack();
    s = FromCodepoints(cps);
    return s;
}

void InsertUtf8AtCaret(Utf8String& s, std::size_t& caretCodepoints, std::size_t& selectionAnchor, const char* utf8) {
    DeleteSelectionIfAny(s, caretCodepoints, selectionAnchor);
    if (utf8 == nullptr || utf8[0] == '\0') {
        return;
    }
    Array<std::uint32_t> cps;
    CollectCodepoints(s, cps);
    Array<std::uint32_t> insCps;
    {
        Utf8String ins(utf8);
        std::uint32_t cp = 0;
        for (auto it = ins.Iterator(); it.NextCodepoint(cp);) {
            insCps.PushBack(cp);
        }
    }
    const std::size_t at = std::min(caretCodepoints, cps.GetSize());
    Array<std::uint32_t> merged;
    merged.Reserve(cps.GetSize() + insCps.GetSize());
    for (std::size_t i = 0; i < at; ++i) {
        merged.PushBack(cps[i]);
    }
    for (std::size_t i = 0; i < insCps.GetSize(); ++i) {
        merged.PushBack(insCps[i]);
    }
    for (std::size_t i = at; i < cps.GetSize(); ++i) {
        merged.PushBack(cps[i]);
    }
    caretCodepoints = at + insCps.GetSize();
    selectionAnchor = caretCodepoints;
    s = FromCodepoints(merged);
}

void InsertUtf8AtCaret(Utf8String& s, std::size_t& caretCodepoints, const char* utf8) {
    InsertUtf8AtCaret(s, caretCodepoints, caretCodepoints, utf8);
}

bool HasTextSelection(const std::size_t caretCodepoints, const std::size_t selectionAnchor) noexcept {
    return caretCodepoints != selectionAnchor;
}

Utf8String Utf8SubstringCodepoints(const Utf8String& s, const std::size_t startCp, const std::size_t endCp) noexcept {
    if (startCp >= endCp) {
        return {};
    }
    Array<std::uint32_t> cps;
    CollectCodepoints(s, cps);
    const std::size_t lo = std::min(startCp, cps.GetSize());
    const std::size_t hi = std::min(endCp, cps.GetSize());
    Array<std::uint32_t> slice;
    for (std::size_t i = lo; i < hi; ++i) {
        slice.PushBack(cps[i]);
    }
    return FromCodepoints(slice);
}

void DeleteSelectionIfAny(Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor) noexcept {
    if (!HasTextSelection(caretCodepoints, selectionAnchor)) {
        return;
    }
    const std::size_t lo = std::min(caretCodepoints, selectionAnchor);
    const std::size_t hi = std::max(caretCodepoints, selectionAnchor);
    Array<std::uint32_t> cps;
    CollectCodepoints(value, cps);
    if (lo >= cps.GetSize()) {
        caretCodepoints = lo;
        selectionAnchor = lo;
        return;
    }
    const std::size_t end = std::min(hi, cps.GetSize());
    Array<std::uint32_t> merged;
    merged.Reserve(cps.GetSize() - (end - lo));
    for (std::size_t i = 0; i < lo; ++i) {
        merged.PushBack(cps[i]);
    }
    for (std::size_t i = end; i < cps.GetSize(); ++i) {
        merged.PushBack(cps[i]);
    }
    caretCodepoints = lo;
    selectionAnchor = lo;
    value = FromCodepoints(merged);
}

bool IsShiftDown(const IInput& in) noexcept {
    return in.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || in.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
}

bool IsCtrlDown(const IInput& in) noexcept {
    return in.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || in.IsKeyDown(GLFW_KEY_RIGHT_CONTROL) ||
           in.IsKeyDown(GLFW_KEY_LEFT_SUPER) || in.IsKeyDown(GLFW_KEY_RIGHT_SUPER);
}

bool ProcessPrintableKeyScan(
        IInput& input, Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor) {
    auto insert = [&](const char* ch) {
        InsertUtf8AtCaret(value, caretCodepoints, selectionAnchor, ch);
        return true;
    };
    if (input.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
        return insert(" ");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_SLASH)) {
        return insert("/");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_PERIOD)) {
        return insert(".");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_COMMA)) {
        return insert(",");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_SEMICOLON)) {
        return insert(IsShiftDown(input) ? ":" : ";");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_APOSTROPHE)) {
        return insert(IsShiftDown(input) ? "\"" : "'");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET)) {
        return insert(IsShiftDown(input) ? "{" : "[");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET)) {
        return insert(IsShiftDown(input) ? "}" : "]");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_BACKSLASH)) {
        return insert(IsShiftDown(input) ? "|" : "\\");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_EQUAL)) {
        return insert(IsShiftDown(input) ? "+" : "=");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_GRAVE_ACCENT)) {
        return insert(IsShiftDown(input) ? "~" : "`");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_MINUS)) {
        return insert(IsShiftDown(input) ? "_" : "-");
    }
    const bool shift = IsShiftDown(input);
    for (int k = GLFW_KEY_A; k <= GLFW_KEY_Z; ++k) {
        if (input.IsKeyPressedThisFrame(k)) {
            char c = static_cast<char>('a' + (k - GLFW_KEY_A));
            if (shift) {
                c = static_cast<char>('A' + (k - GLFW_KEY_A));
            }
            const char buf[2] = {c, '\0'};
            return insert(buf);
        }
    }
    for (int k = GLFW_KEY_0; k <= GLFW_KEY_9; ++k) {
        if (input.IsKeyPressedThisFrame(k)) {
            static const char* const kShiftDigits[10] = {")", "!", "@", "#", "$", "%", "^", "&", "*", "("};
            const char* ch = nullptr;
            char digitBuf[2]{};
            if (shift) {
                ch = kShiftDigits[k - GLFW_KEY_0];
            } else {
                digitBuf[0] = static_cast<char>('0' + (k - GLFW_KEY_0));
                ch = digitBuf;
            }
            return insert(ch);
        }
    }
    return false;
}

bool ProcessTextEditNavigationKeys(
        IInput& input, std::size_t& caretCodepoints, std::size_t& selectionAnchor, const std::size_t codepointCount) {
    const bool shift = IsShiftDown(input);
    if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
        if (caretCodepoints > 0) {
            --caretCodepoints;
        }
        if (!shift) {
            selectionAnchor = caretCodepoints;
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
        if (caretCodepoints < codepointCount) {
            ++caretCodepoints;
        }
        if (!shift) {
            selectionAnchor = caretCodepoints;
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        caretCodepoints = 0;
        if (!shift) {
            selectionAnchor = caretCodepoints;
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_END)) {
        caretCodepoints = codepointCount;
        if (!shift) {
            selectionAnchor = caretCodepoints;
        }
        return true;
    }
    return false;
}

bool ProcessTextEditClipboardKeys(
        IInput& input, Utf8String& value, std::size_t& caretCodepoints, std::size_t& selectionAnchor) {
    if (!IsCtrlDown(input)) {
        return false;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_V)) {
        Utf8String clip;
        if (input.TryGetClipboardUtf8(clip) && !clip.IsEmpty()) {
            InsertUtf8AtCaret(value, caretCodepoints, selectionAnchor, clip.CStr());
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_C)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            const std::size_t lo = std::min(caretCodepoints, selectionAnchor);
            const std::size_t hi = std::max(caretCodepoints, selectionAnchor);
            input.SetClipboardUtf8(Utf8SubstringCodepoints(value, lo, hi));
        } else {
            input.SetClipboardUtf8(value);
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_X)) {
        if (HasTextSelection(caretCodepoints, selectionAnchor)) {
            const std::size_t lo = std::min(caretCodepoints, selectionAnchor);
            const std::size_t hi = std::max(caretCodepoints, selectionAnchor);
            input.SetClipboardUtf8(Utf8SubstringCodepoints(value, lo, hi));
            DeleteSelectionIfAny(value, caretCodepoints, selectionAnchor);
        } else {
            input.SetClipboardUtf8(value);
            value.Clear();
            caretCodepoints = 0;
            selectionAnchor = 0;
        }
        return true;
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_A)) {
        selectionAnchor = 0;
        caretCodepoints = CountCodepoints(value);
        return true;
    }
    return false;
}

}  // namespace Spark::Gui
