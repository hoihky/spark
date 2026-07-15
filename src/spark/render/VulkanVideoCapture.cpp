#include "spark/render/VulkanVideoCapture.hpp"

#include "spark/media/VideoRecorder.hpp"
#include "spark/render/FrameCaptureWatermark.hpp"
#include "spark/render/VulkanRendererGpu.hpp"

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
    return recorder_.Get() != nullptr && recorder_->IsActive();
}

VideoRecorder* VulkanVideoCapture::GetRecorder() noexcept {
    return recorder_.Get();
}

const VideoRecorder* VulkanVideoCapture::GetRecorder() const noexcept {
    return recorder_.Get();
}

void VulkanVideoCapture::Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkFormat swapchainFormat) {
    physicalDevice_ = physicalDevice;
    device_ = device;
    swapchainFormat_ = swapchainFormat;
}

void VulkanVideoCapture::Destroy(VkDevice device) {
    (void)EndRecording();
    if (stagingMapped_ != nullptr) {
        vkUnmapMemory(device, stagingMemory_);
        stagingMapped_ = nullptr;
    }
    if (stagingBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, stagingBuffer_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
    }
    if (stagingMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, stagingMemory_, nullptr);
        stagingMemory_ = VK_NULL_HANDLE;
    }
    stagingBytes_ = 0;
    rowPitch_ = 0;
    bufferExtent_ = {};
    captureQueued_ = false;
    captureExtent_ = {};
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
}

void VulkanVideoCapture::EnsureBuffer(const VkExtent2D extent) {
    if (device_ == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }
    if (bufferExtent_.width == extent.width && bufferExtent_.height == extent.height && stagingBuffer_ != VK_NULL_HANDLE) {
        return;
    }

    if (stagingMapped_ != nullptr) {
        vkUnmapMemory(device_, stagingMemory_);
        stagingMapped_ = nullptr;
    }
    if (stagingBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
    }
    if (stagingMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, stagingMemory_, nullptr);
        stagingMemory_ = VK_NULL_HANDLE;
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice_, &props);
    const VkDeviceSize alignment = props.limits.optimalBufferCopyRowPitchAlignment;
    rowPitch_ = AlignUp(static_cast<VkDeviceSize>(extent.width) * 4U, alignment);
    stagingBytes_ = rowPitch_ * static_cast<VkDeviceSize>(extent.height);

    VulkanRendererGpu::CreateBuffer(
            physicalDevice_,
            device_,
            stagingBytes_,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer_,
            stagingMemory_);
    if (vkMapMemory(device_, stagingMemory_, 0, stagingBytes_, 0, &stagingMapped_) != VK_SUCCESS) {
        throw std::runtime_error("VulkanVideoCapture: vkMapMemory failed");
    }
    bufferExtent_ = extent;
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
    recorder_ = MoveTemp(recorder);
    settings_ = settings;
    recordStartPts_ = 0.0;
    EnsureBuffer(extent);
    return true;
}

bool VulkanVideoCapture::EndRecording() noexcept {
    captureQueued_ = false;
    captureExtent_ = {};
    if (!recorder_) {
        return false;
    }
    const bool ok = recorder_->End();
    recorder_.Reset();
    settings_ = {};
    recordStartPts_ = 0.0;
    scratchBgra_.Clear();
    scratchRgba_.Clear();
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
    captureExtent_ = extent;
    captureQueued_ = true;

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

    vkCmdCopyImageToBuffer(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer_, 1, &region);

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
    if (!captureQueued_ || !IsRecording() || stagingMapped_ == nullptr || captureExtent_.width == 0 ||
        captureExtent_.height == 0) {
        captureQueued_ = false;
        return;
    }

    const std::uint32_t width = captureExtent_.width;
    const std::uint32_t height = captureExtent_.height;
    const std::size_t tightBgraCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    scratchBgra_.Resize(tightBgraCount);

    const auto* src = static_cast<const std::uint8_t*>(stagingMapped_);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(rowPitch_);
        std::memcpy(scratchBgra_.GetData() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U, row, static_cast<std::size_t>(width) * 4U);
    }

    if (settings_.applyWatermark) {
        scratchRgba_.Resize(tightBgraCount);
        ConvertBgraRowsToRgba(scratchBgra_.GetData(), width, height, static_cast<std::size_t>(width) * 4U, scratchRgba_.GetData());
        ApplyFrameCaptureWatermark(scratchRgba_.GetData(), width, height);
        ConvertRgbaToBgraRows(scratchRgba_.GetData(), width, height, scratchBgra_.GetData(), static_cast<std::size_t>(width) * 4U);
    }

    const std::uint32_t outW = recorder_->OutputWidth();
    const std::uint32_t outH = recorder_->OutputHeight();
    if (outW != width || outH != height) {
        const std::size_t scaledCount = static_cast<std::size_t>(outW) * static_cast<std::size_t>(outH) * 4U;
        scratchRgba_.Resize(scaledCount);
        DownscaleBgraBox(scratchBgra_.GetData(), width, height, scratchRgba_.GetData(), outW, outH);
        recorder_->AppendVideoFrameBgra(scratchRgba_.GetData(), outW, outH, ptsSeconds);
    } else {
        recorder_->AppendVideoFrameBgra(scratchBgra_.GetData(), width, height, ptsSeconds);
    }

    captureQueued_ = false;
    captureExtent_ = {};
}

}  // namespace Spark
