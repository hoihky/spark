#include "spark/render/scene/VulkanOffscreenRenderTarget.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/post/VulkanHdrTonemapPass.hpp"

#include <stdexcept>

namespace Spark {

VkFormat VulkanOffscreenRenderTarget::ColorFormatForDesc(const RenderTextureFormat format) noexcept {
    switch (format) {
    case RenderTextureFormat::HdrRGBA16Float:
        return VulkanHdrTonemapPass::kColorFormat;
    case RenderTextureFormat::Rgba8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    default:
        return VulkanHdrTonemapPass::kColorFormat;
    }
}

void VulkanOffscreenRenderTarget::Create(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkRenderPass renderPass,
        const VkFormat depthFormat,
        const RenderTextureDesc& desc) {
    Destroy(device);
    if (renderPass == VK_NULL_HANDLE || desc.width == 0 || desc.height == 0) {
        return;
    }

    extent = {desc.width, desc.height};
    const VkFormat colorFormat = ColorFormatForDesc(desc.colorFormat);

    VulkanRendererGpu::CreateImage(
            physicalDevice,
            device,
            desc.width,
            desc.height,
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            colorImage,
            colorMemory);

    VkImageViewCreateInfo colorViewInfo{};
    colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorViewInfo.image = colorImage;
    colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorViewInfo.format = colorFormat;
    colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorViewInfo.subresourceRange.levelCount = 1;
    colorViewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &colorViewInfo, nullptr, &colorView) != VK_SUCCESS) {
        throw std::runtime_error("VulkanOffscreenRenderTarget: color image view failed");
    }
    colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    Array<VkImageView> attachments{};
    attachments.PushBack(colorView);

    if (desc.includeDepth) {
        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                desc.width,
                desc.height,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage,
                depthMemory);

        VkImageViewCreateInfo depthViewInfo = colorViewInfo;
        depthViewInfo.image = depthImage;
        depthViewInfo.format = depthFormat;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (vkCreateImageView(device, &depthViewInfo, nullptr, &depthView) != VK_SUCCESS) {
            throw std::runtime_error("VulkanOffscreenRenderTarget: depth image view failed");
        }
        depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments.PushBack(depthView);
    }

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = static_cast<std::uint32_t>(attachments.GetSize());
    fbInfo.pAttachments = attachments.GetData();
    fbInfo.width = desc.width;
    fbInfo.height = desc.height;
    fbInfo.layers = 1;
    if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("VulkanOffscreenRenderTarget: framebuffer failed");
    }
}

void VulkanOffscreenRenderTarget::Destroy(const VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }
    if (depthView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthView, nullptr);
        depthView = VK_NULL_HANDLE;
    }
    if (depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }
    if (depthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, depthMemory, nullptr);
        depthMemory = VK_NULL_HANDLE;
    }
    if (colorView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, colorView, nullptr);
        colorView = VK_NULL_HANDLE;
    }
    if (colorImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, colorImage, nullptr);
        colorImage = VK_NULL_HANDLE;
    }
    if (colorMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, colorMemory, nullptr);
        colorMemory = VK_NULL_HANDLE;
    }
    extent = {0, 0};
    colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    allocatedToken = 0;
}

}  // namespace Spark
