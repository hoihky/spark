#pragma once

#include "spark/engine/IInput.hpp"

#include <GLFW/glfw3.h>

struct GLFWwindow;

namespace Spark {

class Window;

/** GLFW-backed <c>IInput</c> (Adapter). Lives in the engine module, not tied to <c>Engine::Impl</c>. */
class GlfwInput final : public IInput {
public:
    explicit GlfwInput(GLFWwindow* window);

    void WireToWindow(Window& win);
    void BeginFrame();

    [[nodiscard]] bool IsKeyDown(int keyCode) const override;
    [[nodiscard]] bool IsKeyPressedThisFrame(int keyCode) const override;
    [[nodiscard]] float GetMouseDeltaX() const override;
    [[nodiscard]] float GetMouseDeltaY() const override;
    [[nodiscard]] bool IsMouseButtonDown(int button) const override;
    [[nodiscard]] bool IsMouseButtonPressedThisFrame(int button) const override;
    [[nodiscard]] bool IsMouseButtonReleasedThisFrame(int button) const override;
    void GetCursorFramebufferPixels(float& outX, float& outY, int drawableWidth, int drawableHeight) const override;
    void SetCursorCaptured(bool capture) override;
    [[nodiscard]] bool IsCursorCaptured() const override;
    [[nodiscard]] float GetScrollDeltaY() const override;
    [[nodiscard]] bool TryGetClipboardUtf8(Utf8String& out) const override;
    void SetClipboardUtf8(const Utf8String& text) const override;

private:
    void Clear();

    GLFWwindow* window = nullptr;
    bool curr[GLFW_KEY_LAST + 1U]{};
    bool prev[GLFW_KEY_LAST + 1U]{};
    bool mouseCurr[GLFW_MOUSE_BUTTON_LAST + 1U]{};
    bool mousePrev[GLFW_MOUSE_BUTTON_LAST + 1U]{};
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    double cursorXWin = 0.0;
    double cursorYWin = 0.0;
    float mouseDeltaX = 0.0F;
    float mouseDeltaY = 0.0F;
    bool cursorCaptured = false;
    double scrollAccumY = 0.0;
    float scrollDeltaThisFrame = 0.0F;
};

}  // namespace Spark
