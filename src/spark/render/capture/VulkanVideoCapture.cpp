#include "spark/render/capture/VulkanVideoCapture.hpp"

#include "spark/media/VideoRecorder.hpp"
#include "spark/render/capture/FrameCaptureWatermark.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
    if (alignment == 0) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

}  // namespace

bool VulkanVideoCapture::IsRecording() const noexcept {
    return recorder.Get() != nullptr && recorder->IsActive();
}

VideoRecorder* VulkanVideoCapture::GetRecorder() noexcept {
    return recorder.Get();
}

const VideoRecorder* VulkanVideoCapture::GetRecorder() const noexcept {
    return recorder.Get();
}

void VulkanVideoCapture::Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkFormat swapchainFormat) {
    this->physicalDevice = physicalDevice;
    this->device = device;
    this->swapchainFormat = swapchainFormat;
}

void VulkanVideoCapture::Destroy(VkDevice device) {
    (void)EndRecording();
    if (stagingMapped != nullptr) {
        vkUnmapMemory(device, stagingMemory);
        stagingMapped = nullptr;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    if (stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, stagingMemory, nullptr);
        stagingMemory = VK_NULL_HANDLE;
    }
    stagingBytes = 0;
    rowPitch = 0;
    bufferExtent = {};
    captureQueued = false;
    captureExtent = {};
    this->device = VK_NULL_HANDLE;
    this->physicalDevice = VK_NULL_HANDLE;
}

void VulkanVideoCapture::EnsureBuffer(const VkExtent2D extent) {
    if (device == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }
    if (bufferExtent.width == extent.width && bufferExtent.height == extent.height && stagingBuffer != VK_NULL_HANDLE) {
        return;
    }

    if (stagingMapped != nullptr) {
        vkUnmapMemory(device, stagingMemory);
        stagingMapped = nullptr;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    if (stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, stagingMemory, nullptr);
        stagingMemory = VK_NULL_HANDLE;
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    const VkDeviceSize alignment = props.limits.optimalBufferCopyRowPitchAlignment;
    rowPitch = AlignUp(static_cast<VkDeviceSize>(extent.width) * 4U, alignment);
    stagingBytes = rowPitch * static_cast<VkDeviceSize>(extent.height);

    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            stagingBytes,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);
    if (vkMapMemory(device, stagingMemory, 0, stagingBytes, 0, &stagingMapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanVideoCapture: vkMapMemory failed");
    }
    bufferExtent = extent;
}

bool VulkanVideoCapture::BeginRecording(
        UniquePtr<VideoRecorder> recorder,
        const VideoRecordingSettings& settings,
        const VkExtent2D extent) {
    (void)EndRecording();
    if (!recorder || extent.width == 0 || extent.height == 0) {
        return false;
    }
    if (!recorder->Begin(settings, extent.width, extent.height)) {
        return false;
    }
    this->recorder = MoveTemp(recorder);
    this->settings = settings;
    recordStartPts = 0.0;
    EnsureBuffer(extent);
    return true;
}

bool VulkanVideoCapture::EndRecording() noexcept {
    captureQueued = false;
    captureExtent = {};
    if (!recorder) {
        return false;
    }
    const bool ok = recorder->End();
    recorder.Reset();
    settings = {};
    recordStartPts = 0.0;
    scratchBgra.Clear();
    scratchRgba.Clear();
    return ok;
}

void VulkanVideoCapture::RecordCopyFromSwapchain(
        VkCommandBuffer commandBuffer,
        VkImage swapchainImage,
        const VkExtent2D extent) {
    if (!IsRecording() || swapchainImage == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }

    EnsureBuffer(extent);
    captureExtent = extent;
    captureQueued = true;

    VkImageMemoryBarrier toSrc{};
    toSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toSrc.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toSrc.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.image = swapchainImage;
    toSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toSrc.subresourceRange.levelCount = 1;
    toSrc.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toSrc);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = extent.width;
    region.imageExtent.height = extent.height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    VkImageMemoryBarrier toPresent{};
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toPresent.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toPresent.dstAccessMask = 0;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = swapchainImage;
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toPresent);
}

void VulkanVideoCapture::TryCommitFrameAfterFence(const double ptsSeconds) noexcept {
    if (!captureQueued || !IsRecording() || stagingMapped == nullptr || captureExtent.width == 0 ||
        captureExtent.height == 0) {
        captureQueued = false;
        return;
    }

    const std::uint32_t width = captureExtent.width;
    const std::uint32_t height = captureExtent.height;
    const std::size_t tightBgraCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    scratchBgra.Resize(tightBgraCount);

    const auto* src = static_cast<const std::uint8_t*>(stagingMapped);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(rowPitch);
        std::memcpy(scratchBgra.GetData() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U, row, static_cast<std::size_t>(width) * 4U);
    }

    if (settings.applyWatermark) {
        scratchRgba.Resize(tightBgraCount);
        ConvertBgraRowsToRgba(scratchBgra.GetData(), width, height, static_cast<std::size_t>(width) * 4U, scratchRgba.GetData());
        ApplyFrameCaptureWatermark(scratchRgba.GetData(), width, height);
        ConvertRgbaToBgraRows(scratchRgba.GetData(), width, height, scratchBgra.GetData(), static_cast<std::size_t>(width) * 4U);
    }

    const std::uint32_t outW = recorder->OutputWidth();
    const std::uint32_t outH = recorder->OutputHeight();
    if (outW != width || outH != height) {
        const std::size_t scaledCount = static_cast<std::size_t>(outW) * static_cast<std::size_t>(outH) * 4U;
        scratchRgba.Resize(scaledCount);
        DownscaleBgraBox(scratchBgra.GetData(), width, height, scratchRgba.GetData(), outW, outH);
        recorder->AppendVideoFrameBgra(scratchRgba.GetData(), outW, outH, ptsSeconds);
    } else {
        recorder->AppendVideoFrameBgra(scratchBgra.GetData(), width, height, ptsSeconds);
    }

    captureQueued = false;
    captureExtent = {};
}

}  // namespace Spark
