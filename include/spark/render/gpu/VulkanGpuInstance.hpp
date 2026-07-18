#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

class Window;

namespace VulkanRendererGpu {

/**
 * Vulkan instance concerns: validation availability, required instance extensions, and the
 * debug utils messenger used when validation is enabled (single responsibility vs device/memory).
 */
class VulkanGpuInstance {
public:
#ifndef NDEBUG
    static constexpr bool kEnableValidationLayers = true;
#else
    static constexpr bool kEnableValidationLayers = false;
#endif
    static constexpr const char* kKhronosValidationLayerName = "VK_LAYER_KHRONOS_validation";

    [[nodiscard]] static bool CheckValidationLayerSupport();

    [[nodiscard]] static Array<const char*> GetRequiredInstanceExtensions(const Window& appWindow);

    [[nodiscard]] static VkResult CreateDebugUtilsMessenger(
            VkInstance vkInstance,
            const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
            const VkAllocationCallbacks* allocator,
            VkDebugUtilsMessengerEXT* outMessenger);

    static void DestroyDebugUtilsMessenger(
            VkInstance vkInstance,
            VkDebugUtilsMessengerEXT messenger,
            const VkAllocationCallbacks* allocator);
};

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanGpuDefaultDebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

}  // namespace VulkanRendererGpu

}  // namespace Spark
