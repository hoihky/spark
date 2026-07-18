#include "spark/render/present/VulkanDepthResources.hpp"

#include "spark/render/gpu/VulkanGpuBufferImage.hpp"

#include <stdexcept>

namespace Spark {

void VulkanDepthResources::Create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D extent,
        VkFormat depthFormat) {
    Destroy(device);
    format = depthFormat;
    VulkanRendererGpu::VulkanGpuBufferImage::CreateImage(
            physicalDevice,
            device,
            extent.width,
            extent.height,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            memory);

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateImageView (depth) failed");
    }
    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanDepthResources::Destroy(VkDevice device) noexcept {
    if (view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

}  // namespace Spark
