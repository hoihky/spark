#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * Copies the presented swapchain image to a host buffer and saves PNG.
 * macOS screen-capture shortcuts often record MoltenVK windows as black; use F12 in-engine instead.
 */
class VulkanScreenshotCapture {
public:
    void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkFormat swapchainFormat);
    void Destroy(VkDevice device);

    void EnsureBuffer(VkExtent2D extent);

    /** Queue a PNG path (UTF-8). Captured on the next completed frame. */
    void RequestSave(const char* pathUtf8);

    [[nodiscard]] bool HasPendingCapture() const noexcept { return pendingCapture_; }

    void RecordCopyFromSwapchain(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkExtent2D extent);

    /** Call after the submit fence signals. Returns whether a file was written. */
    bool TrySavePendingPng();

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;

    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory_ = VK_NULL_HANDLE;
    void* stagingMapped_ = nullptr;
    VkDeviceSize stagingBytes_ = 0;
    VkDeviceSize rowPitch_ = 0;
    VkExtent2D bufferExtent_{};

    char pendingPath_[512]{};
    bool pendingCapture_ = false;
    VkExtent2D captureExtent_{};
};

}  // namespace Spark
