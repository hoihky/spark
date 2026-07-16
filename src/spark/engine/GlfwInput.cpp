#include "spark/engine/GlfwInput.hpp"

#include "spark/render/Window.hpp"

#include <cmath>
#include <cstring>

#if defined(__APPLE__)
#define SPARK_GLFW_USE_CURSOR_CALLBACK 0
#else
#define SPARK_GLFW_USE_CURSOR_CALLBACK 1
#endif

namespace Spark {

GlfwInput::GlfwInput(GLFWwindow* w) : window(w) {
    Clear();
}

void GlfwInput::WireToWindow([[maybe_unused]] Window& win) {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
    win.SetCursorPosCallback([this](double x, double y) {
        cursorXWin = x;
        cursorYWin = y;
    });
#endif
    win.SetScrollCallback([this](double /*xoffset*/, double yoffset) { scrollAccumY += yoffset; });
    glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
}

void GlfwInput::BeginFrame() {
    scrollDeltaThisFrame = static_cast<float>(scrollAccumY);
    scrollAccumY = 0.0;
    for (int k = 0; k <= GLFW_KEY_LAST; ++k) {
        prev[static_cast<unsigned>(k)] = curr[static_cast<unsigned>(k)];
        const int s = glfwGetKey(window, k);
        curr[static_cast<unsigned>(k)] = (s == GLFW_PRESS || s == GLFW_REPEAT);
    }
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
        mousePrev[static_cast<unsigned>(b)] = mouseCurr[static_cast<unsigned>(b)];
        mouseCurr[static_cast<unsigned>(b)] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
    }

    double mx = 0.0;
    double my = 0.0;
    if (cursorCaptured) {
        glfwGetCursorPos(window, &mx, &my);
        mouseDeltaX = static_cast<float>(mx - lastMouseX);
        mouseDeltaY = static_cast<float>(my - lastMouseY);
        int winW = 0;
        int winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        if (winW > 0 && winH > 0) {
            const double cx = static_cast<double>(winW) * 0.5;
            const double cy = static_cast<double>(winH) * 0.5;
            glfwSetCursorPos(window, cx, cy);
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        constexpr float kCapturedMouseDeadZone = 0.12F;
        if (std::fabs(mouseDeltaX) < kCapturedMouseDeadZone) {
            mouseDeltaX = 0.0F;
        }
        if (std::fabs(mouseDeltaY) < kCapturedMouseDeadZone) {
            mouseDeltaY = 0.0F;
        }
        if (glfwRawMouseMotionSupported() != 0) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    } else {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
        mx = cursorXWin;
        my = cursorYWin;
#else
        glfwGetCursorPos(window, &mx, &my);
#endif
        mouseDeltaX = static_cast<float>(mx - lastMouseX);
        mouseDeltaY = static_cast<float>(my - lastMouseY);
        lastMouseX = mx;
        lastMouseY = my;
    }
}

bool GlfwInput::IsKeyDown(int keyCode) const {
    if (keyCode < 0 || keyCode > GLFW_KEY_LAST) {
        return false;
    }
    return curr[static_cast<unsigned>(keyCode)];
}

bool GlfwInput::IsKeyPressedThisFrame(int keyCode) const {
    if (keyCode < 0 || keyCode > GLFW_KEY_LAST) {
        return false;
    }
    const unsigned u = static_cast<unsigned>(keyCode);
    return curr[u] && !prev[u];
}

float GlfwInput::GetMouseDeltaX() const {
    return mouseDeltaX;
}

float GlfwInput::GetMouseDeltaY() const {
    return mouseDeltaY;
}

bool GlfwInput::IsMouseButtonDown(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }
    return mouseCurr[static_cast<unsigned>(button)];
}

bool GlfwInput::IsMouseButtonPressedThisFrame(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }
    const unsigned u = static_cast<unsigned>(button);
    return mouseCurr[u] && !mousePrev[u];
}

bool GlfwInput::IsMouseButtonReleasedThisFrame(int button) const {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }
    const unsigned u = static_cast<unsigned>(button);
    return !mouseCurr[u] && mousePrev[u];
}

void GlfwInput::GetCursorFramebufferPixels(float& outX, float& outY, int drawableWidth, int drawableHeight) const {
    int winW = 0;
    int winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    int fbW = 0;
    int fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    if (drawableWidth > 0 && drawableHeight > 0) {
        fbW = drawableWidth;
        fbH = drawableHeight;
    }
    double cx = 0.0;
    double cy = 0.0;
    if (cursorCaptured) {
        glfwGetCursorPos(window, &cx, &cy);
    } else {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
        cx = cursorXWin;
        cy = cursorYWin;
#else
        glfwGetCursorPos(window, &cx, &cy);
#endif
    }
    if (winW > 0 && winH > 0 && fbW > 0 && fbH > 0) {
        outX = static_cast<float>(cx * static_cast<double>(fbW) / static_cast<double>(winW));
        outY = static_cast<float>(cy * static_cast<double>(fbH) / static_cast<double>(winH));
    } else {
        outX = static_cast<float>(cx);
        outY = static_cast<float>(cy);
    }
}

void GlfwInput::SetCursorCaptured(bool capture) {
    if (cursorCaptured == capture) {
        if (!capture) {
#if !SPARK_GLFW_USE_CURSOR_CALLBACK
            glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
#endif
        }
        return;
    }
    cursorCaptured = capture;
    glfwSetInputMode(window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported() != 0) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, capture ? GLFW_TRUE : GLFW_FALSE);
    }
    int winW = 0;
    int winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW > 0 && winH > 0) {
        if (capture) {
            const double cx = static_cast<double>(winW) * 0.5;
            const double cy = static_cast<double>(winH) * 0.5;
            glfwSetCursorPos(window, cx, cy);
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else {
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
    }
    if (!capture) {
        glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
    }
    mouseDeltaX = 0.0F;
    mouseDeltaY = 0.0F;
}

bool GlfwInput::IsCursorCaptured() const {
    return cursorCaptured;
}

float GlfwInput::GetScrollDeltaY() const {
    return scrollDeltaThisFrame;
}

bool GlfwInput::TryGetClipboardUtf8(Utf8String& out) const {
    const char* c = glfwGetClipboardString(window);
    if (c == nullptr || c[0] == '\0') {
        return false;
    }
    out = Utf8String(c);
    return true;
}

void GlfwInput::SetClipboardUtf8(const Utf8String& text) const {
    glfwSetClipboardString(window, text.CStr());
}

void GlfwInput::Clear() {
    std::memset(curr, 0, sizeof(curr));
    std::memset(prev, 0, sizeof(prev));
    std::memset(mouseCurr, 0, sizeof(mouseCurr));
    std::memset(mousePrev, 0, sizeof(mousePrev));
}

}  // namespace Spark
