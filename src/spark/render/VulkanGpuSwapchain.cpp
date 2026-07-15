#include "spark/render/VulkanGpuSwapchain.hpp"

#include "spark/core/Array.hpp"

#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdint>

namespace Spark {
namespace VulkanRendererGpu {

namespace {

template<typename T>
[[nodiscard]] constexpr const T& ClampRef(const T& v, const T& lo, const T& hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace

SwapchainSupportDetails VulkanGpuSwapchain::QuerySwapchainSupport(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    std::uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.Resize(static_cast<std::size_t>(formatCount));
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice, surface, &formatCount, details.formats.GetData());
    }

    std::uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
    if (presentCount != 0) {
        details.presentModes.Resize(static_cast<std::size_t>(presentCount));
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, surface, &presentCount, details.presentModes.GetData());
    }
    return details;
}

VkSurfaceFormatKHR VulkanGpuSwapchain::ChooseSwapSurfaceFormat(const Array<VkSurfaceFormatKHR>& formats) {
    for (std::size_t i = 0; i < formats.GetSize(); ++i) {
        const VkSurfaceFormatKHR& format = formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];
}

VkPresentModeKHR VulkanGpuSwapchain::ChooseSwapPresentMode(const Array<VkPresentModeKHR>& presentModes) {
    for (std::size_t i = 0; i < presentModes.GetSize(); ++i) {
        const VkPresentModeKHR presentMode = presentModes[i];
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanGpuSwapchain::ChooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        GLFWwindow* glfwWindow) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    VkExtent2D extent{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    extent.width = ClampRef(
            extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
    extent.height = ClampRef(
            extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
    return extent;
}

}  // namespace VulkanRendererGpu
}  // namespace Spark
