#include "spark/render/scene/VulkanSceneOpaqueBackground.hpp"

#include "spark/render/post/VulkanHdrTonemapPass.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"

#include <stdexcept>

namespace Spark {

void VulkanSceneOpaqueBackground::Recreate(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkExtent2D extent,
        const VkFormat sceneDepthFormatIn,
        const std::uint32_t framesInFlight) {
    Destroy(device);
    depthFormat = sceneDepthFormatIn;
    if (extent.width == 0 || extent.height == 0 || framesInFlight == 0 || depthFormat == VK_FORMAT_UNDEFINED) {
        return;
    }

    flights.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < flights.GetSize(); ++fi) {
        FlightTarget& flight = flights[fi];
        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                extent.width,
                extent.height,
                VulkanHdrTonemapPass::kColorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                flight.colorImage,
                flight.colorMemory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = flight.colorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VulkanHdrTonemapPass::kColorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &viewInfo, nullptr, &flight.colorView) != VK_SUCCESS) {
            throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateImageView (color) failed");
        }
        flight.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                extent.width,
                extent.height,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                flight.depthImage,
                flight.depthMemory);

        VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT) {
            depthAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image = flight.depthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat;
        depthViewInfo.subresourceRange.aspectMask = depthAspect;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &depthViewInfo, nullptr, &flight.depthView) != VK_SUCCESS) {
            throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateImageView (depth) failed");
        }
        flight.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &colorSampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateSampler (color) failed");
    }
    if (vkCreateSampler(device, &samplerInfo, nullptr, &depthSampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateSampler (depth) failed");
    }
}

void VulkanSceneOpaqueBackground::InitializeLayouts(
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue,
        const VkExtent2D extent) {
    if (extent.width == 0 || extent.height == 0 || flights.IsEmpty()) {
        return;
    }

    VulkanRendererGpu::RunOneTimeCommands(device, commandPool, graphicsQueue, [&](const VkCommandBuffer commandBuffer) {
        for (std::size_t fi = 0; fi < flights.GetSize(); ++fi) {
            FlightTarget& flight = flights[fi];
            if (flight.colorImage == VK_NULL_HANDLE) {
                continue;
            }

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.image = flight.colorImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);

            VkClearColorValue clearColor{};
            clearColor.float32[3] = 1.0F;
            VkImageSubresourceRange clearRange{};
            clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            clearRange.levelCount = 1;
            clearRange.layerCount = 1;
            vkCmdClearColorImage(
                    commandBuffer,
                    flight.colorImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &clearColor,
                    1,
                    &clearRange);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);
            flight.colorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (flight.depthImage == VK_NULL_HANDLE) {
                continue;
            }

            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.image = flight.depthImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);

            VkClearDepthStencilValue clearDepth{};
            clearDepth.depth = 1.0F;
            VkImageSubresourceRange depthClearRange{};
            depthClearRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthClearRange.levelCount = 1;
            depthClearRange.layerCount = 1;
            vkCmdClearDepthStencilImage(
                    commandBuffer,
                    flight.depthImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &clearDepth,
                    1,
                    &depthClearRange);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);
            flight.depthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    });
}

void VulkanSceneOpaqueBackground::Destroy(const VkDevice device) noexcept {
    if (colorSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, colorSampler, nullptr);
        colorSampler = VK_NULL_HANDLE;
    }
    if (depthSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, depthSampler, nullptr);
        depthSampler = VK_NULL_HANDLE;
    }
    for (std::size_t fi = 0; fi < flights.GetSize(); ++fi) {
        FlightTarget& flight = flights[fi];
        if (flight.colorView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, flight.colorView, nullptr);
            flight.colorView = VK_NULL_HANDLE;
        }
        if (flight.colorImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, flight.colorImage, nullptr);
            flight.colorImage = VK_NULL_HANDLE;
        }
        if (flight.colorMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, flight.colorMemory, nullptr);
            flight.colorMemory = VK_NULL_HANDLE;
        }
        if (flight.depthView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, flight.depthView, nullptr);
            flight.depthView = VK_NULL_HANDLE;
        }
        if (flight.depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, flight.depthImage, nullptr);
            flight.depthImage = VK_NULL_HANDLE;
        }
        if (flight.depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, flight.depthMemory, nullptr);
            flight.depthMemory = VK_NULL_HANDLE;
        }
        flight.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        flight.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    flights.Clear();
    depthFormat = VK_FORMAT_UNDEFINED;
}

void VulkanSceneOpaqueBackground::RecordCopyFromHdrColor(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VkImage hdrColorImage,
        const VkExtent2D extent) {
    if (!HasFlight(frameIndex) || hdrColorImage == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }

    FlightTarget& scratch = flights[frameIndex];

    VkImageMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image = hdrColorImage;
    barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[0].subresourceRange.levelCount = 1;
    barriers[0].subresourceRange.layerCount = 1;

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[1].srcAccessMask =
            (scratch.colorLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].oldLayout =
            scratch.colorLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_UNDEFINED : scratch.colorLayout;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = scratch.colorImage;
    barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[1].subresourceRange.levelCount = 1;
    barriers[1].subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            2,
            barriers);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = {extent.width, extent.height, 1};
    vkCmdCopyImage(
            commandBuffer,
            hdrColorImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            scratch.colorImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            2,
            barriers);

    scratch.colorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanSceneOpaqueBackground::RecordCopyFromSceneDepth(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VkImage sceneDepthImage,
        const VkExtent2D extent) {
    if (!HasFlight(frameIndex) || sceneDepthImage == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }

    FlightTarget& scratch = flights[frameIndex];
    if (scratch.depthImage == VK_NULL_HANDLE) {
        return;
    }

    VkImageMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image = sceneDepthImage;
    barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barriers[0].subresourceRange.levelCount = 1;
    barriers[0].subresourceRange.layerCount = 1;

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[1].srcAccessMask =
            (scratch.depthLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].oldLayout =
            scratch.depthLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_UNDEFINED : scratch.depthLayout;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = scratch.depthImage;
    barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barriers[1].subresourceRange.levelCount = 1;
    barriers[1].subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barriers[0]);

    VkPipelineStageFlags depthScratchSrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    if (scratch.depthLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        depthScratchSrcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vkCmdPipelineBarrier(
            commandBuffer,
            depthScratchSrcStage,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barriers[1]);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = {extent.width, extent.height, 1};
    vkCmdCopyImage(
            commandBuffer,
            sceneDepthImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            scratch.depthImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            2,
            barriers);

    scratch.depthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkImageView VulkanSceneOpaqueBackground::ColorView(const std::uint32_t frameIndex) const noexcept {
    return frameIndex < flights.GetSize() ? flights[frameIndex].colorView : VK_NULL_HANDLE;
}

VkImageView VulkanSceneOpaqueBackground::DepthView(const std::uint32_t frameIndex) const noexcept {
    return frameIndex < flights.GetSize() ? flights[frameIndex].depthView : VK_NULL_HANDLE;
}

}  // namespace Spark
