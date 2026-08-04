#pragma once

#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {
class UiCanvasComponent;
}

namespace Spark::Ui {

/** Stable element identity for per-id state and factory lookups. */
using UiElementId = Utf8String;

/** Screen-space axis-aligned rectangle (framebuffer pixels, Y downward). */
struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    [[nodiscard]] bool Contains(const float px, const float py) const noexcept {
        return px >= x && py >= y && px < x + width && py < y + height;
    }

    [[nodiscard]] Rect Inset(const float pad) const noexcept {
        return {x + pad, y + pad, width - 2.0F * pad, height - 2.0F * pad};
    }
};

struct UiSize {
    float width = 0.0F;
    float height = 0.0F;
};

struct UiMeasureConstraints {
    float minWidth = 0.0F;
    float minHeight = 0.0F;
    float maxWidth = 1.0e9F;
    float maxHeight = 1.0e9F;
};

/** Normalized pointer sample for one frame (from IInput + cursor position). */
struct UiFrameInput {
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
    float scrollDeltaY = 0.0F;
};

enum class TextOverflow : std::uint8_t {
    Visible = 0,
    Clip,
    Ellipsis,
};

enum class TextWrap : std::uint8_t {
    NoWrap = 0,
    WordWrap,
};

struct TextLayout {
    TextOverflow overflow = TextOverflow::Ellipsis;
    TextWrap wrap = TextWrap::NoWrap;
    int maxLines = 0;
};

/** Lightweight callback without std::function. */
struct UiVoidCallback {
    void (*fn)(void* userData) = nullptr;
    void* userData = nullptr;

    void Invoke() const noexcept {
        if (fn != nullptr) {
            fn(userData);
        }
    }

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }
};

struct UiFloatCallback {
    void (*fn)(void* userData, float value) = nullptr;
    void* userData = nullptr;

    void Invoke(const float value) const noexcept {
        if (fn != nullptr) {
            fn(userData, value);
        }
    }

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }
};

struct UiBoolCallback {
    void (*fn)(void* userData, bool value) = nullptr;
    void* userData = nullptr;

    void Invoke(const bool value) const noexcept {
        if (fn != nullptr) {
            fn(userData, value);
        }
    }

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }
};

struct UiIntCallback {
    void (*fn)(void* userData, int value) = nullptr;
    void* userData = nullptr;

    void Invoke(const int value) const noexcept {
        if (fn != nullptr) {
            fn(userData, value);
        }
    }

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }
};

struct UiFrameVoidCallback {
    void (*fn)(void* userData, const UiFrameInput& input, ::Spark::UiCanvasComponent& canvas) = nullptr;
    void* userData = nullptr;

    void Invoke(const UiFrameInput& input, ::Spark::UiCanvasComponent& canvas) const noexcept {
        if (fn != nullptr) {
            fn(userData, input, canvas);
        }
    }

    [[nodiscard]] bool IsBound() const noexcept { return fn != nullptr; }
};

}  // namespace Spark::Ui
