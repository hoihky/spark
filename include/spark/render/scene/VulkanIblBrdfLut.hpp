#pragma once

#include <vulkan/vulkan.h>

namespace Spark {

/** Pre-integrated GGX BRDF split-sum LUT (RG16F, 512×512). Bound at scene descriptor set binding 12. */
class VulkanIblBrdfLut {
public:
    void Create(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkCommandPool commandPool,
            VkQueue graphicsQueue);
    void Destroy(VkDevice device) noexcept;

    [[nodiscard]] VkImageView View() const noexcept { return imageView; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler; }

private:
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};

}  // namespace Spark
