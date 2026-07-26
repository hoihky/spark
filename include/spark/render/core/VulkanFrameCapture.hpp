#pragma once

#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/capture/VulkanScreenshotCapture.hpp"
#include "spark/render/capture/VulkanVideoCapture.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

class VideoRecorder;

/** Swapchain-tied PNG screenshot and MP4 video readback (shared staging setup). */
class VulkanFrameCapture {
public:
    void RecreateSwapchainResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkFormat swapchainFormat,
            VkExtent2D extent);
    void Destroy(VkDevice device) noexcept;

    void RequestScreenshotSave(const char* pathUtf8);
    [[nodiscard]] bool BeginVideoRecording(
            UniquePtr<VideoRecorder> recorder,
            const VideoRecordingSettings& settings,
            VkExtent2D extent);
    void EndVideoRecording() noexcept;
    [[nodiscard]] bool IsVideoRecording() const noexcept;
    [[nodiscard]] VideoRecorder* GetRecorder() noexcept;

    [[nodiscard]] bool NeedsPostSubmitWork() const noexcept;

    void RecordCopyFromSwapchain(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkExtent2D extent,
            std::uint32_t flightIndex);

    /** After the per-flight fence has been waited on at frame start. */
    void OnFlightFenceSignaled(std::uint32_t waitedFlightIndex) noexcept;

    void FlushPendingCaptures(VkDevice device, const VkFence* inFlightFences) noexcept;

private:
    VulkanScreenshotCapture screenshotCapture;
    VulkanVideoCapture videoCapture;
};

}  // namespace Spark
