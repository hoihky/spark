#include "spark/render/platform/Window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>

namespace Spark {

Window::Window() {
    if (!glfwInit()) {
        throw std::runtime_error("glfwInit failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    glfwWindow = glfwCreateWindow(DefaultWidth, DefaultHeight, "Spark", nullptr, nullptr);
    if (glfwWindow == nullptr) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(
            glfwWindow,
            +[](GLFWwindow* w, int width, int height) {
                auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
                if (self != nullptr) {
                    self->HandleFramebufferResize(width, height);
                }
            });
}

Window::~Window() {
    if (glfwWindow != nullptr) {
        glfwDestroyWindow(glfwWindow);
        glfwWindow = nullptr;
    }
    glfwTerminate();
}

void Window::PollEvents() const { glfwPollEvents(); }

bool Window::ShouldClose() const { return glfwWindowShouldClose(glfwWindow) != 0; }

void Window::GetFramebufferSize(int& width, int& height) const {
    glfwGetFramebufferSize(glfwWindow, &width, &height);
}

void Window::GetContentScale(float& scaleX, float& scaleY) const {
    scaleX = 1.0F;
    scaleY = 1.0F;
    if (glfwWindow != nullptr) {
        glfwGetWindowContentScale(glfwWindow, &scaleX, &scaleY);
    }
}

void Window::SetFramebufferResizeCallback(FramebufferResizeCallback callback) {
    resizeCallback = std::move(callback);
}

void Window::HandleFramebufferResize(int width, int height) {
    if (resizeCallback) {
        resizeCallback(width, height);
    }
}

void Window::OnCursorPosForward(GLFWwindow* window, double x, double y) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->cursorPosCallback) {
        self->cursorPosCallback(x, y);
    }
}

void Window::SetCursorPosCallback(CursorPosCallback callback) {
    cursorPosCallback = std::move(callback);
    glfwSetCursorPosCallback(glfwWindow, cursorPosCallback ? OnCursorPosForward : nullptr);
}

void Window::OnScrollForward(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->scrollCallback) {
        self->scrollCallback(xoffset, yoffset);
    }
}

void Window::SetScrollCallback(ScrollCallback callback) {
    scrollCallback = std::move(callback);
    glfwSetScrollCallback(glfwWindow, scrollCallback ? OnScrollForward : nullptr);
}

Array<const char*> Window::RequiredVulkanInstanceExtensions() const {
    std::uint32_t count = 0;
    const char** names = glfwGetRequiredInstanceExtensions(&count);
    if (names == nullptr || count == 0) {
        throw std::runtime_error("glfwGetRequiredInstanceExtensions returned no extensions");
    }
    Array<const char*> out;
    out.Reserve(static_cast<std::size_t>(count));
    for (std::uint32_t i = 0; i < count; ++i) {
        out.PushBack(names[i]);
    }
    return out;
}

void Window::CreateVulkanSurface(void* instancePtr, void* outSurface) const {
    const auto vkInstance = static_cast<VkInstance>(instancePtr);
    auto* surfaceOut = static_cast<VkSurfaceKHR*>(outSurface);
    if (glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, surfaceOut) != VK_SUCCESS) {
        throw std::runtime_error("glfwCreateWindowSurface failed");
    }
}

}  // namespace Spark
