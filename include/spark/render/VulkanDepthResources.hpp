#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Depth attachment backing the presentation render pass (image + device-local memory + view).
 */
class VulkanDepthResources {
public:
    VkFormat format{};
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    void Create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat depthFormat);
    void Destroy(VkDevice device) noexcept;
};

}  // namespace Spark
