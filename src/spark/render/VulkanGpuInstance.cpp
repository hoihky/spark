#include "spark/render/VulkanGpuInstance.hpp"

#include "spark/render/Window.hpp"

#include <cstdio>
#include <cstring>
#include <vulkan/vulkan.h>

namespace Spark {
namespace VulkanRendererGpu {

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanGpuDefaultDebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData) {
    (void)messageType;
    (void)userData;
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && callbackData != nullptr &&
        callbackData->pMessage != nullptr) {
        std::fprintf(stderr, "[Vulkan] %s\n", callbackData->pMessage);
    }
    return VK_FALSE;
}

bool VulkanGpuInstance::CheckValidationLayerSupport() {
    std::uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    Array<VkLayerProperties> available;
    available.Resize(static_cast<std::size_t>(layerCount));
    vkEnumerateInstanceLayerProperties(&layerCount, available.GetData());
    for (std::size_t i = 0; i < available.GetSize(); ++i) {
        if (std::strcmp(available[i].layerName, VulkanGpuInstance::kKhronosValidationLayerName) == 0) {
            return true;
        }
    }
    return false;
}

Array<const char*> VulkanGpuInstance::GetRequiredInstanceExtensions(const Window& appWindow) {
    Array<const char*> extensions = appWindow.RequiredVulkanInstanceExtensions();
#ifdef SPARK_PLATFORM_APPLE
    extensions.PushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    if constexpr (VulkanGpuInstance::kEnableValidationLayers) {
        extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

VkResult VulkanGpuInstance::CreateDebugUtilsMessenger(
        VkInstance vkInstance,
        const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* outMessenger) {
    const auto createFn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (createFn != nullptr) {
        return createFn(vkInstance, createInfo, allocator, outMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanGpuInstance::DestroyDebugUtilsMessenger(
        VkInstance vkInstance,
        VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* allocator) {
    const auto destroyFn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyFn != nullptr) {
        destroyFn(vkInstance, messenger, allocator);
    }
}

}  // namespace VulkanRendererGpu
}  // namespace Spark
