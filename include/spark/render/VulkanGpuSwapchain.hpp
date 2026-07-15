#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Spark {
namespace VulkanRendererGpu {

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    Array<VkSurfaceFormatKHR> formats;
    Array<VkPresentModeKHR> presentModes;
};

/**
 * Swapchain surface formats, present modes, and extent selection (isolated from buffer/memory work).
 */
class VulkanGpuSwapchain {
public:
    [[nodiscard]] static SwapchainSupportDetails QuerySwapchainSupport(
            VkPhysicalDevice physicalDevice,
            VkSurfaceKHR surface);

    [[nodiscard]] static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const Array<VkSurfaceFormatKHR>& formats);

    [[nodiscard]] static VkPresentModeKHR ChooseSwapPresentMode(const Array<VkPresentModeKHR>& presentModes);

    [[nodiscard]] static VkExtent2D ChooseSwapExtent(
            const VkSurfaceCapabilitiesKHR& capabilities,
            GLFWwindow* glfwWindow);
};

}  // namespace VulkanRendererGpu
}  // namespace Spark
