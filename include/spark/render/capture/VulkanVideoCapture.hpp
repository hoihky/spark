#pragma once

#include "spark/core/Array.hpp"
#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VideoRecorder;

/**
 * Continuous swapchain readback for MP4 recording (same image as PNG screenshots).
 */
class VulkanVideoCapture {
public:
    void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkFormat swapchainFormat);
    void Destroy(VkDevice device);

    void EnsureBuffer(VkExtent2D extent);

    [[nodiscard]] bool IsRecording() const noexcept;

    [[nodiscard]] VideoRecorder* GetRecorder() noexcept;
    [[nodiscard]] const VideoRecorder* GetRecorder() const noexcept;

    bool BeginRecording(UniquePtr<VideoRecorder> recorder, const VideoRecordingSettings& settings, VkExtent2D extent);
    bool EndRecording() noexcept;

    void RecordCopyFromSwapchain(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkExtent2D extent);

    /** After submit fence signals. @p ptsSeconds wall-clock presentation time for this frame. */
    void TryCommitFrameAfterFence(double ptsSeconds) noexcept;

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

    bool captureQueued = false;
    VkExtent2D captureExtent{};
    VideoRecordingSettings settings{};
    UniquePtr<VideoRecorder> recorder;
    Array<std::uint8_t> scratchBgra;
    Array<std::uint8_t> scratchRgba;
    double recordStartPts = 0.0;
};

}  // namespace Spark
