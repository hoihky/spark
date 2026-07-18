#include "spark/render/gpu/VulkanGpuPhysicalDevice.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/gpu/VulkanGpuSwapchain.hpp"

#include <cstring>

namespace Spark {
namespace VulkanRendererGpu {

QueueFamilyIndices VulkanGpuPhysicalDevice::FindQueueFamilies(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    Array<VkQueueFamilyProperties> props;
    props.Resize(static_cast<std::size_t>(count));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, props.GetData());

    for (std::uint32_t i = 0; i < count; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.hasGraphicsFamily = true;
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
            indices.hasPresentFamily = true;
        }
        if (indices.Complete()) {
            break;
        }
    }
    return indices;
}

bool VulkanGpuPhysicalDevice::DeviceExtensionSupported(
        VkPhysicalDevice physicalDevice,
        const char* extensionName) {
    std::uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    Array<VkExtensionProperties> props;
    props.Resize(static_cast<std::size_t>(count));
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, props.GetData());
    for (std::size_t i = 0; i < props.GetSize(); ++i) {
        if (std::strcmp(props[i].extensionName, extensionName) == 0) {
            return true;
        }
    }
    return false;
}

bool VulkanGpuPhysicalDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    const QueueFamilyIndices queueFamilies = FindQueueFamilies(physicalDevice, surface);
    if (!queueFamilies.Complete()) {
        return false;
    }
    const SwapchainSupportDetails swapchainSupport = VulkanGpuSwapchain::QuerySwapchainSupport(physicalDevice, surface);
    if (swapchainSupport.formats.IsEmpty() || swapchainSupport.presentModes.IsEmpty()) {
        return false;
    }
    if (!DeviceExtensionSupported(physicalDevice, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return false;
    }
#ifdef SPARK_PLATFORM_APPLE
    if (!DeviceExtensionSupported(physicalDevice, "VK_KHR_portability_subset")) {
        return false;
    }
#endif
    return true;
}

}  // namespace VulkanRendererGpu
}  // namespace Spark
