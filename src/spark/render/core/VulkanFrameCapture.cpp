#include "spark/render/core/VulkanFrameCapture.hpp"

#include "spark/media/VideoRecorder.hpp"
#include "spark/render/core/VulkanFrameSync.hpp"

namespace Spark {

void VulkanFrameCapture::RecreateSwapchainResources(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkFormat swapchainFormat,
        VkExtent2D extent) {
    screenshotCapture.Create(physicalDevice, device, swapchainFormat);
    screenshotCapture.EnsureBuffer(extent);
    videoCapture.Create(physicalDevice, device, swapchainFormat);
    if (!videoCapture.IsRecording()) {
        videoCapture.EnsureBuffer(extent);
    }
}

void VulkanFrameCapture::Destroy(VkDevice device) noexcept {
    screenshotCapture.Destroy(device);
    videoCapture.Destroy(device);
}

void VulkanFrameCapture::RequestScreenshotSave(const char* pathUtf8) {
    screenshotCapture.RequestSave(pathUtf8);
}

bool VulkanFrameCapture::BeginVideoRecording(
        UniquePtr<VideoRecorder> recorder,
        const VideoRecordingSettings& settings,
        VkExtent2D extent) {
    return videoCapture.BeginRecording(MoveTemp(recorder), settings, extent);
}

void VulkanFrameCapture::EndVideoRecording() noexcept {
    videoCapture.EndRecording();
}

bool VulkanFrameCapture::IsVideoRecording() const noexcept {
    return videoCapture.IsRecording();
}

VideoRecorder* VulkanFrameCapture::GetRecorder() noexcept {
    return videoCapture.GetRecorder();
}

bool VulkanFrameCapture::NeedsPostSubmitWork() const noexcept {
    return screenshotCapture.HasPendingCapture() || videoCapture.IsRecording();
}

void VulkanFrameCapture::RecordCopyFromSwapchain(
        VkCommandBuffer commandBuffer,
        VkImage swapchainImage,
        VkExtent2D extent,
        const std::uint32_t flightIndex) {
    videoCapture.RecordCopyFromSwapchain(commandBuffer, swapchainImage, extent, flightIndex);
    screenshotCapture.RecordCopyFromSwapchain(commandBuffer, swapchainImage, extent, flightIndex);
}

void VulkanFrameCapture::OnFlightFenceSignaled(const std::uint32_t waitedFlightIndex) noexcept {
    (void)screenshotCapture.TrySavePendingPngForFlight(waitedFlightIndex);
    if (videoCapture.IsRecording()) {
        videoCapture.TryCommitFlightCapture(waitedFlightIndex);
    }
}

void VulkanFrameCapture::FlushPendingCaptures(VkDevice device, const VkFence* inFlightFences) noexcept {
    if (videoCapture.IsRecording()) {
        videoCapture.FlushPendingCaptures(device, inFlightFences);
    }
    if (screenshotCapture.HasPendingCapture() && inFlightFences != nullptr) {
        for (std::uint32_t i = 0; i < VulkanFrameSync::kMaxFramesInFlight; ++i) {
            vkWaitForFences(device, 1, &inFlightFences[i], VK_TRUE, UINT64_MAX);
            (void)screenshotCapture.TrySavePendingPngForFlight(i);
        }
    }
}

}  // namespace Spark
