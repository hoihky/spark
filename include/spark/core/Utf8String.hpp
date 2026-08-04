#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Owns a null-terminated UTF-8 byte sequence. Mutations keep a trailing '\0' for CStr().
 */
class Utf8String {
public:
    Utf8String() { bytes.PushBack('\0'); }

    explicit Utf8String(const char* utf8) {
        if (utf8 != nullptr) {
            while (*utf8 != '\0') {
                bytes.PushBack(*utf8++);
            }
        }
        bytes.PushBack('\0');
    }

    Utf8String(const Utf8String&) = default;
    Utf8String& operator=(const Utf8String&) = default;
    Utf8String(Utf8String&&) = default;
    Utf8String& operator=(Utf8String&&) = default;

    [[nodiscard]] const char* CStr() const noexcept { return bytes.GetData(); }
    [[nodiscard]] char* GetMutableBuffer() noexcept { return bytes.GetData(); }

    /** Byte length excluding the trailing null terminator. */
    [[nodiscard]] std::size_t ByteLength() const noexcept {
        const std::size_t n = bytes.GetSize();
        return n <= 1 ? 0 : n - 1;
    }

    [[nodiscard]] bool IsEmpty() const noexcept { return ByteLength() == 0; }

    void Clear() {
        bytes.Clear();
        bytes.PushBack('\0');
    }

    void AppendUtf8(const char* utf8) {
        StripTrailingNull();
        if (utf8 != nullptr) {
            while (*utf8 != '\0') {
                bytes.PushBack(*utf8++);
            }
        }
        bytes.PushBack('\0');
    }

    void AppendUtf8(const Utf8String& other) { AppendUtf8(other.CStr()); }

    /** Appends one Unicode code point as UTF-8 (U+0000 .. U+10FFFF). Invalid values are replaced with U+FFFD. */
    void AppendCodepoint(std::uint32_t cp) {
        StripTrailingNull();
        cp = SanitizeCodepoint(cp);
        if (cp <= 0x7F) {
            bytes.PushBack(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            bytes.PushBack(static_cast<char>(0xC0 | static_cast<char>(cp >> 6)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>(cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            bytes.PushBack(static_cast<char>(0xE0 | static_cast<char>(cp >> 12)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>((cp >> 6) & 0x3F)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>(cp & 0x3F)));
        } else {
            bytes.PushBack(static_cast<char>(0xF0 | static_cast<char>(cp >> 18)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>((cp >> 12) & 0x3F)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>((cp >> 6) & 0x3F)));
            bytes.PushBack(static_cast<char>(0x80 | static_cast<char>(cp & 0x3F)));
        }
        bytes.PushBack('\0');
    }

    [[nodiscard]] bool operator==(const Utf8String& other) const noexcept {
        if (bytes.GetSize() != other.bytes.GetSize()) {
            return false;
        }
        for (std::size_t i = 0; i < bytes.GetSize(); ++i) {
            if (bytes[i] != other.bytes[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool operator!=(const Utf8String& other) const noexcept { return !(*this == other); }

    /** Walks UTF-8 code units; use NextCodepoint for decoding. */
    class ConstIterator {
    public:
        ConstIterator() = default;
        explicit ConstIterator(const char* p, const char* end) : ptr(p), end(end) {}

        [[nodiscard]] const char* GetPointer() const noexcept { return ptr; }

        /** Returns false if invalid sequence or past end. */
        bool NextCodepoint(std::uint32_t& outCp) noexcept {
            if (ptr == nullptr || ptr >= end || *ptr == '\0') {
                return false;
            }
            const auto u0 = static_cast<unsigned char>(*ptr);
            std::uint32_t cp = 0;
            std::size_t extra = 0;
            if (u0 < 0x80) {
                cp = u0;
                extra = 0;
            } else if ((u0 >> 5) == 0x6) {
                cp = u0 & 0x1F;
                extra = 1;
            } else if ((u0 >> 4) == 0xE) {
                cp = u0 & 0x0F;
                extra = 2;
            } else if ((u0 >> 3) == 0x1E) {
                cp = u0 & 0x07;
                extra = 3;
            } else {
                ++ptr;
                outCp = 0xFFFD;
                return true;
            }
            if (ptr + extra >= end) {
                ptr = end;
                outCp = 0xFFFD;
                return true;
            }
            for (std::size_t i = 1; i <= extra; ++i) {
                const auto c = static_cast<unsigned char>(ptr[i]);
                if ((c >> 6) != 2) {
                    ptr += 1;
                    outCp = 0xFFFD;
                    return true;
                }
                cp = (cp << 6) | (c & 0x3F);
            }
            if (extra == 1 && cp < 0x80) {
                ptr += 1 + extra;
                outCp = 0xFFFD;
                return true;
            }
            if (extra == 2 && cp < 0x800) {
                ptr += 1 + extra;
                outCp = 0xFFFD;
                return true;
            }
            if (extra == 3 && cp < 0x10000) {
                ptr += 1 + extra;
                outCp = 0xFFFD;
                return true;
            }
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                ptr += 1 + extra;
                outCp = 0xFFFD;
                return true;
            }
            ptr += 1 + extra;
            outCp = cp;
            return true;
        }

    private:
        const char* ptr = nullptr;
        const char* end = nullptr;
    };

    [[nodiscard]] ConstIterator Iterator() const noexcept {
        const char* p = CStr();
        return ConstIterator(p, p + bytes.GetSize());
    }

private:
    Array<char> bytes;

    void StripTrailingNull() {
        if (!bytes.IsEmpty() && bytes.GetLast() == '\0') {
            bytes.PopBack();
        }
    }

    static std::uint32_t SanitizeCodepoint(std::uint32_t cp) noexcept {
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return 0xFFFD;
        }
        return cp;
    }
};

/** Non-owning reference to a null-terminated UTF-8 byte sequence. */
class Utf8StringView {
public:
    Utf8StringView() = default;

    constexpr Utf8StringView(const char* utf8) noexcept : ptr(utf8 != nullptr ? utf8 : "") {}

    Utf8StringView(const Utf8String& s) noexcept : ptr(s.CStr()) {}

    [[nodiscard]] const char* CStr() const noexcept { return ptr; }

    [[nodiscard]] bool IsEmpty() const noexcept { return ptr == nullptr || *ptr == '\0'; }

private:
    const char* ptr = "";
};

struct Utf8StringHash {
    [[nodiscard]] std::size_t operator()(const Utf8String& s) const noexcept {
        std::size_t h = 5381;
        for (const char* p = s.CStr(); *p != '\0'; ++p) {
            h = ((h << 5) + h) + static_cast<unsigned char>(*p);
        }
        return h;
    }
};

struct Utf8StringEqual {
    [[nodiscard]] bool operator()(const Utf8String& a, const Utf8String& b) const noexcept {
        return a == b;
    }
};

}  // namespace Spark
