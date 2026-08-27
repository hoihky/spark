#include "spark/render/scene/VulkanSceneOpaqueBackground.hpp"

#include "spark/render/post/VulkanHdrTonemapPass.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"

#include <stdexcept>

namespace Spark {

void VulkanSceneOpaqueBackground::Recreate(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkExtent2D extent,
        const std::uint32_t framesInFlight) {
    Destroy(device);
    if (extent.width == 0 || extent.height == 0 || framesInFlight == 0) {
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
                flight.image,
                flight.memory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = flight.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VulkanHdrTonemapPass::kColorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &viewInfo, nullptr, &flight.view) != VK_SUCCESS) {
            throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateImageView failed");
        }
        flight.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanSceneOpaqueBackground: vkCreateSampler failed");
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
            if (flight.image == VK_NULL_HANDLE) {
                continue;
            }

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = flight.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;
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
                    flight.image,
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

            flight.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    });
}

void VulkanSceneOpaqueBackground::Destroy(const VkDevice device) noexcept {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    for (std::size_t fi = 0; fi < flights.GetSize(); ++fi) {
        FlightTarget& flight = flights[fi];
        if (flight.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, flight.view, nullptr);
            flight.view = VK_NULL_HANDLE;
        }
        if (flight.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, flight.image, nullptr);
            flight.image = VK_NULL_HANDLE;
        }
        if (flight.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, flight.memory, nullptr);
            flight.memory = VK_NULL_HANDLE;
        }
        flight.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    flights.Clear();
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
            (scratch.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].oldLayout =
            scratch.layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_UNDEFINED : scratch.layout;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = scratch.image;
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
            scratch.image,
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

    scratch.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkImageView VulkanSceneOpaqueBackground::View(const std::uint32_t frameIndex) const noexcept {
    return frameIndex < flights.GetSize() ? flights[frameIndex].view : VK_NULL_HANDLE;
}

}  // namespace Spark
