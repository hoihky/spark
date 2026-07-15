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
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;

    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory_ = VK_NULL_HANDLE;
    void* stagingMapped_ = nullptr;
    VkDeviceSize stagingBytes_ = 0;
    VkDeviceSize rowPitch_ = 0;
    VkExtent2D bufferExtent_{};

    bool captureQueued_ = false;
    VkExtent2D captureExtent_{};
    VideoRecordingSettings settings_{};
    UniquePtr<VideoRecorder> recorder_;
    Array<std::uint8_t> scratchBgra_;
    Array<std::uint8_t> scratchRgba_;
    double recordStartPts_ = 0.0;
};

}  // namespace Spark
