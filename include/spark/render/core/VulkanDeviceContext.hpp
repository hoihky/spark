#pragma once

#include "spark/render/gpu/VulkanGpuPhysicalDevice.hpp"
#include "spark/render/present/VulkanPresentationSwapchain.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

class Window;

/**
 * Owns Vulkan instance, surface, physical/logical device, queues, and swapchain color images.
 * Separates GPU device lifecycle from render-pass orchestration in <c>VulkanRenderer</c>.
 */
class VulkanDeviceContext {
public:
    explicit VulkanDeviceContext(Window& window);
    ~VulkanDeviceContext();

    VulkanDeviceContext(const VulkanDeviceContext&) = delete;
    VulkanDeviceContext& operator=(const VulkanDeviceContext&) = delete;
    VulkanDeviceContext(VulkanDeviceContext&&) = delete;
    VulkanDeviceContext& operator=(VulkanDeviceContext&&) = delete;

    [[nodiscard]] VkInstance GetInstance() const noexcept { return instance; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const noexcept { return physicalDevice; }
    [[nodiscard]] VkDevice GetDevice() const noexcept { return device; }
    [[nodiscard]] VkSurfaceKHR GetSurface() const noexcept { return surface; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const noexcept { return graphicsQueue; }
    [[nodiscard]] VkQueue GetPresentQueue() const noexcept { return presentQueue; }
    [[nodiscard]] const VulkanRendererGpu::QueueFamilyIndices& GetQueueFamilies() const noexcept {
        return queueFamilies;
    }

    [[nodiscard]] VulkanPresentationSwapchain& GetSwapchain() noexcept { return swapchain; }
    [[nodiscard]] const VulkanPresentationSwapchain& GetSwapchain() const noexcept { return swapchain; }

    void WaitDeviceIdle() const;
    void DestroySwapchain() noexcept;
    /** Waits for idle, destroys any existing swapchain, then creates a new one for the bound window. */
    void RecreateSwapchain();
    [[nodiscard]] bool TryGetDrawableSize(int& outWidth, int& outHeight) const noexcept;

private:
    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchainImages();

    Window* boundWindow = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VulkanRendererGpu::QueueFamilyIndices queueFamilies{};

    VulkanPresentationSwapchain swapchain;
};

}  // namespace Spark
