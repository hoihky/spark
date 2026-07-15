#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Swapchain color images and views (no framebuffers — those depend on render pass + depth).
 */
class VulkanPresentationSwapchain {
public:
    VkSwapchainKHR khr = VK_NULL_HANDLE;
    Array<VkImage> images;
    Array<VkImageView> imageViews;
    VkFormat imageFormat{};
    VkExtent2D extent{};

    void Destroy(VkDevice device) noexcept;
};

}  // namespace Spark
