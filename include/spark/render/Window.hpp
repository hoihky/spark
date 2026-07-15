#pragma once

#include "spark/core/Array.hpp"

#include <functional>

struct GLFWwindow;

namespace Spark {

class Window {
public:
    static constexpr int DefaultWidth = 1280;
    static constexpr int DefaultHeight = 720;

    Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;
    ~Window();

    [[nodiscard]] GLFWwindow* Handle() const { return glfwWindow; }
    void PollEvents() const;
    [[nodiscard]] bool ShouldClose() const;
    void GetFramebufferSize(int& width, int& height) const;

    /** GLFW content scale (1.0 = 96 DPI); use max of x/y for uniform UI scaling. */
    void GetContentScale(float& scaleX, float& scaleY) const;

    using FramebufferResizeCallback = std::function<void(int width, int height)>;
    void SetFramebufferResizeCallback(FramebufferResizeCallback callback);

    /** Cursor position in the same window coordinate space as glfwGetCursorPos (when supported). */
    using CursorPosCallback = std::function<void(double x, double y)>;
    void SetCursorPosCallback(CursorPosCallback callback);

    using ScrollCallback = std::function<void(double xoffset, double yoffset)>;
    void SetScrollCallback(ScrollCallback callback);

    /** Called from GLFW framebuffer callback (invokes user callback if set). */
    void HandleFramebufferResize(int width, int height);

    [[nodiscard]] Array<const char*> RequiredVulkanInstanceExtensions() const;
    void CreateVulkanSurface(void* instance, void* outSurface) const;

private:
    static void OnCursorPosForward(GLFWwindow* window, double x, double y);
    static void OnScrollForward(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* glfwWindow = nullptr;
    FramebufferResizeCallback resizeCallback;
    CursorPosCallback cursorPosCallback;
    ScrollCallback scrollCallback;
};

}  // namespace Spark
