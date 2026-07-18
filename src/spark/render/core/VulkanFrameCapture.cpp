#include "spark/render/core/VulkanFrameCapture.hpp"

#include "spark/media/VideoRecorder.hpp"

namespace Spark {

void VulkanFrameCapture::RecreateSwapchainResources(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkFormat swapchainFormat,
        VkExtent2D extent) {
    screenshotCapture.Create(physicalDevice, device, swapchainFormat);
    screenshotCapture.EnsureBuffer(extent);
    videoCapture.Create(physicalDevice, device, swapchainFormat);
    videoCapture.EnsureBuffer(extent);
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
        VkExtent2D extent) {
    screenshotCapture.RecordCopyFromSwapchain(commandBuffer, swapchainImage, extent);
    videoCapture.RecordCopyFromSwapchain(commandBuffer, swapchainImage, extent);
}

void VulkanFrameCapture::TrySavePendingPng() {
    screenshotCapture.TrySavePendingPng();
}

void VulkanFrameCapture::TryCommitFrameAfterFence(double ptsSeconds) noexcept {
    if (videoCapture.IsRecording()) {
        videoCapture.TryCommitFrameAfterFence(ptsSeconds);
    }
}

}  // namespace Spark
