#include "spark/render/sprites2d/VulkanCompositeTonemapBlit.hpp"

#include "spark/render/core/VulkanRendererGpu.hpp"

#include <algorithm>
#include <stdexcept>

namespace Spark {

void VulkanCompositeTonemapBlit::BindDevice(const VkPhysicalDevice physicalDeviceIn, const VkDevice deviceIn) noexcept {
    physicalDevice = physicalDeviceIn;
    device = deviceIn;
}

void VulkanCompositeTonemapBlit::Create(const VkDevice deviceIn) {
    Destroy(deviceIn);
    if (deviceIn == VK_NULL_HANDLE) {
        return;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VulkanOffscreenLdrRenderPass::kColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &colorAttachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &dep;
    if (vkCreateRenderPass(deviceIn, &rpInfo, nullptr, &tonemapOutputRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("VulkanCompositeTonemapBlit: render pass failed");
    }
}

void VulkanCompositeTonemapBlit::DestroyScratch(const VkDevice deviceIn) noexcept {
    if (deviceIn == VK_NULL_HANDLE) {
        return;
    }
    if (scratchFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(deviceIn, scratchFramebuffer, nullptr);
        scratchFramebuffer = VK_NULL_HANDLE;
    }
    if (scratchColorView != VK_NULL_HANDLE) {
        vkDestroyImageView(deviceIn, scratchColorView, nullptr);
        scratchColorView = VK_NULL_HANDLE;
    }
    if (scratchColorImage != VK_NULL_HANDLE) {
        vkDestroyImage(deviceIn, scratchColorImage, nullptr);
        scratchColorImage = VK_NULL_HANDLE;
    }
    if (scratchColorMemory != VK_NULL_HANDLE) {
        vkFreeMemory(deviceIn, scratchColorMemory, nullptr);
        scratchColorMemory = VK_NULL_HANDLE;
    }
    scratchExtent = {0, 0};
    scratchColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanCompositeTonemapBlit::Destroy(const VkDevice deviceIn) noexcept {
    DestroyScratch(deviceIn);
    if (deviceIn != VK_NULL_HANDLE && tonemapOutputRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(deviceIn, tonemapOutputRenderPass, nullptr);
        tonemapOutputRenderPass = VK_NULL_HANDLE;
    }
    physicalDevice = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

void VulkanCompositeTonemapBlit::EnsureScratch(const VkExtent2D extent) {
    if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE || tonemapOutputRenderPass == VK_NULL_HANDLE) {
        return;
    }
    if (extent.width == 0U || extent.height == 0U) {
        return;
    }
    if (scratchColorImage != VK_NULL_HANDLE && scratchExtent.width == extent.width &&
        scratchExtent.height == extent.height) {
        return;
    }

    DestroyScratch(device);

    scratchExtent = extent;
    VulkanRendererGpu::CreateImage(
            physicalDevice,
            device,
            extent.width,
            extent.height,
            VulkanOffscreenLdrRenderPass::kColorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            scratchColorImage,
            scratchColorMemory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = scratchColorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VulkanOffscreenLdrRenderPass::kColorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &scratchColorView) != VK_SUCCESS) {
        throw std::runtime_error("VulkanCompositeTonemapBlit: scratch color view failed");
    }

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = tonemapOutputRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &scratchColorView;
    fbInfo.width = extent.width;
    fbInfo.height = extent.height;
    fbInfo.layers = 1;
    if (vkCreateFramebuffer(device, &fbInfo, nullptr, &scratchFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("VulkanCompositeTonemapBlit: scratch framebuffer failed");
    }
    scratchColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanCompositeTonemapBlit::RecordTonemap(
        const VkCommandBuffer commandBuffer,
        VulkanHdrTonemapPass& tonemapPass,
        const std::uint32_t flightIndex,
        const float exposure) noexcept {
    if (commandBuffer == VK_NULL_HANDLE || scratchFramebuffer == VK_NULL_HANDLE ||
        scratchExtent.width == 0U || scratchExtent.height == 0U) {
        return;
    }

    if (scratchColorLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout =
                scratchColorLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_UNDEFINED : scratchColorLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = scratchColorImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);
        scratchColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    tonemapPass.RecordTonemapToOffscreenFramebuffer(
            commandBuffer,
            flightIndex,
            tonemapOutputRenderPass,
            scratchFramebuffer,
            scratchExtent,
            exposure);

    scratchColorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    {
        VkImageMemoryBarrier toCopy{};
        toCopy.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toCopy.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toCopy.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        toCopy.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toCopy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        toCopy.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toCopy.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toCopy.image = scratchColorImage;
        toCopy.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toCopy.subresourceRange.levelCount = 1;
        toCopy.subresourceRange.layerCount = 1;
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
                &toCopy);
        scratchColorLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
}

}  // namespace Spark