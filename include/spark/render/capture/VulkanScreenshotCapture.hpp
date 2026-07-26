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

    [[nodiscard]] bool HasPendingCapture() const noexcept { return pendingCapture; }

    void RecordCopyFromSwapchain(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkExtent2D extent,
            std::uint32_t flightIndex);

    /** Call after <c>flightIndex</c>'s in-flight fence has signaled. Returns whether a file was written. */
    bool TrySavePendingPngForFlight(std::uint32_t flightIndex);

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    void* stagingMapped = nullptr;
    VkDeviceSize stagingBytes = 0;
    VkDeviceSize rowPitch = 0;
    VkExtent2D bufferExtent{};

    char pendingPath[512]{};
    bool pendingCapture = false;
    bool copyQueued = false;
    std::uint32_t captureFlightIndex = 0;
    VkExtent2D captureExtent{};
};

}  // namespace Spark
