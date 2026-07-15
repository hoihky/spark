#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Spark {
namespace VulkanRendererGpu {

struct QueueFamilyIndices {
    std::uint32_t graphicsFamily = 0;
    std::uint32_t presentFamily = 0;
    bool hasGraphicsFamily = false;
    bool hasPresentFamily = false;

    [[nodiscard]] bool Complete() const noexcept { return hasGraphicsFamily && hasPresentFamily; }
};

/**
 * Physical-device selection: queue families, extension checks, and swapchain readiness (delegates
 * swapchain capability queries to VulkanGpuSwapchain).
 */
class VulkanGpuPhysicalDevice {
public:
    [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    [[nodiscard]] static bool DeviceExtensionSupported(VkPhysicalDevice physicalDevice, const char* extensionName);

    [[nodiscard]] static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
};

}  // namespace VulkanRendererGpu
}  // namespace Spark
