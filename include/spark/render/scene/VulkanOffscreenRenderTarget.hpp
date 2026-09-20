#pragma once

#include "spark/scene/render/RenderTextureDesc.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** GPU color (+ optional depth) images and framebuffer for one offscreen target. */
class VulkanOffscreenRenderTarget {
public:
    void Create(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkRenderPass renderPass,
            VkFormat depthFormat,
            const RenderTextureDesc& desc);
    void Destroy(VkDevice device) noexcept;

    [[nodiscard]] bool IsAllocated() const noexcept { return colorImage != VK_NULL_HANDLE; }
    [[nodiscard]] VkFramebuffer Framebuffer() const noexcept { return framebuffer; }
    [[nodiscard]] VkImageView ColorView() const noexcept { return colorView; }
    [[nodiscard]] VkImageView DepthView() const noexcept { return depthView; }
    [[nodiscard]] VkExtent2D Extent() const noexcept { return extent; }
    [[nodiscard]] VkImageLayout ColorLayout() const noexcept { return colorLayout; }
    [[nodiscard]] VkImageLayout DepthLayout() const noexcept { return depthLayout; }
    [[nodiscard]] std::uint64_t AllocatedToken() const noexcept { return allocatedToken; }
    void SetAllocatedToken(std::uint64_t token) noexcept { allocatedToken = token; }

private:
    [[nodiscard]] static VkFormat ColorFormatForDesc(RenderTextureFormat format) noexcept;

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory = VK_NULL_HANDLE;
    VkImageView colorView = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkExtent2D extent{};
    VkImageLayout colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::uint64_t allocatedToken = 0;
};

}  // namespace Spark
