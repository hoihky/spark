#include "spark/render/VulkanScreenshotCapture.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/FrameCaptureWatermark.hpp"
#include "spark/render/VulkanRendererGpu.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

void VulkanScreenshotCapture::Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkFormat swapchainFormat) {
    physicalDevice_ = physicalDevice;
    device_ = device;
    swapchainFormat_ = swapchainFormat;
}

void VulkanScreenshotCapture::Destroy(VkDevice device) {
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
    pendingCapture_ = false;
    pendingPath_[0] = '\0';
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
}

void VulkanScreenshotCapture::EnsureBuffer(const VkExtent2D extent) {
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
        throw std::runtime_error("VulkanScreenshotCapture: vkMapMemory failed");
    }
    bufferExtent_ = extent;
}

void VulkanScreenshotCapture::RequestSave(const char* pathUtf8) {
    if (pathUtf8 == nullptr || pathUtf8[0] == '\0') {
        return;
    }
    std::snprintf(pendingPath_, sizeof(pendingPath_), "%s", pathUtf8);
    pendingCapture_ = true;
}

void VulkanScreenshotCapture::RecordCopyFromSwapchain(
        VkCommandBuffer commandBuffer,
        VkImage swapchainImage,
        const VkExtent2D extent) {
    if (!pendingCapture_ || swapchainImage == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }

    EnsureBuffer(extent);
    captureExtent_ = extent;

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

bool VulkanScreenshotCapture::TrySavePendingPng() {
    if (!pendingCapture_ || stagingMapped_ == nullptr || captureExtent_.width == 0 || captureExtent_.height == 0) {
        pendingCapture_ = false;
        return false;
    }

    const std::uint32_t width = captureExtent_.width;
    const std::uint32_t height = captureExtent_.height;
    const auto* src = static_cast<const std::uint8_t*>(stagingMapped_);
    const std::size_t rgbaCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    Array<std::uint8_t> rgba;
    rgba.Resize(rgbaCount);

    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(rowPitch_);
        std::uint8_t* dstRow = rgba.GetData() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint8_t b = row[x * 4 + 0];
            const std::uint8_t g = row[x * 4 + 1];
            const std::uint8_t r = row[x * 4 + 2];
            const std::uint8_t a = row[x * 4 + 3];
            dstRow[x * 4 + 0] = r;
            dstRow[x * 4 + 1] = g;
            dstRow[x * 4 + 2] = b;
            dstRow[x * 4 + 3] = a;
        }
    }

    ApplyFrameCaptureWatermark(rgba.GetData(), width, height);

    const int ok = stbi_write_png(
            pendingPath_,
            static_cast<int>(width),
            static_cast<int>(height),
            4,
            rgba.GetData(),
            static_cast<int>(width) * 4);
    const bool saved = ok != 0;
    if (saved) {
        std::fprintf(stderr, "Spark: screenshot saved to %s\n", pendingPath_);
    } else {
        std::fprintf(stderr, "Spark: screenshot failed (could not write %s)\n", pendingPath_);
    }

    pendingCapture_ = false;
    pendingPath_[0] = '\0';
    captureExtent_ = {};
    return saved;
}

}  // namespace Spark
