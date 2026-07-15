#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/**
 * Abstract input (keyboard/mouse/gamepad can be implemented per platform).
 * Key codes are GLFW values when using GlfwInput (see GLFW key constants).
 */
class IInput {
public:
    virtual ~IInput() = default;

    [[nodiscard]] virtual bool IsKeyDown(int keyCode) const = 0;
    [[nodiscard]] virtual bool IsKeyPressedThisFrame(int keyCode) const = 0;

    [[nodiscard]] virtual float GetMouseDeltaX() const { return 0.0F; }
    [[nodiscard]] virtual float GetMouseDeltaY() const { return 0.0F; }
    /** GLFW_MOUSE_BUTTON_* (e.g. 0 = left). */
    [[nodiscard]] virtual bool IsMouseButtonDown(int button) const {
        (void)button;
        return false;
    }

    [[nodiscard]] virtual bool IsMouseButtonPressedThisFrame(int button) const {
        (void)button;
        return false;
    }

    [[nodiscard]] virtual bool IsMouseButtonReleasedThisFrame(int button) const {
        (void)button;
        return false;
    }

    /** Vertical scroll wheel delta for this frame (typ. ±1 from GLFW per notch). Default 0. */
    [[nodiscard]] virtual float GetScrollDeltaY() const { return 0.0F; }

    /**
     * Cursor in drawable pixels (top-left origin, Y down). When drawableWidth/Height > 0, use the same size as
     * IEngineContext::GetFramebufferSize (e.g. swapchain on Apple/MoltenVK); otherwise backend chooses (e.g. GLFW fb).
     */
    virtual void GetCursorFramebufferPixels(
            float& outX,
            float& outY,
            int drawableWidth = 0,
            int drawableHeight = 0) const {
        (void)drawableWidth;
        (void)drawableHeight;
        outX = 0.0F;
        outY = 0.0F;
    }

    virtual void SetCursorCaptured(bool capture) { (void)capture; }
    [[nodiscard]] virtual bool IsCursorCaptured() const { return false; }

    /** OS clipboard as UTF-8 when supported (GLFW backend). */
    [[nodiscard]] virtual bool TryGetClipboardUtf8(Utf8String& out) const {
        (void)out;
        return false;
    }

    virtual void SetClipboardUtf8(const Utf8String& text) const { (void)text; }
};

}  // namespace Spark
