#pragma once

#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Main swapchain render pass (color + depth); independent from pipeline layout and shaders.
 */
class VulkanPresentRenderPass {
public:
    VkRenderPass vkPass = VK_NULL_HANDLE;

    /** When <c>depthFormat</c> is <c>VK_FORMAT_UNDEFINED</c>, the pass is color-only (tonemap + UI). */
    void Create(VkDevice device, VkFormat swapchainImageFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
    void Destroy(VkDevice device) noexcept;
};

}  // namespace Spark
