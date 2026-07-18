#pragma once

#include "spark/core/Array.hpp"

#include <cstddef>
#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Per–swapchain-image framebuffers binding the presentation render pass.
 */
class VulkanPresentationFramebuffers {
public:
    Array<VkFramebuffer> buffers;

    void Create(
            VkDevice device,
            VkRenderPass renderPass,
            VkExtent2D extent,
            const Array<VkImageView>& colorImageViews,
            VkImageView depthImageView = VK_NULL_HANDLE);

    void Destroy(VkDevice device) noexcept;
};

}  // namespace Spark
