#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Spark {
namespace VulkanRendererGpu {

/**
 * Device-local memory, buffers, images, samplers, and simple image barriers used by the renderer.
 */
class VulkanGpuBufferImage {
public:
    [[nodiscard]] static std::uint32_t FindMemoryType(
            VkPhysicalDevice physicalDevice,
            std::uint32_t typeFilter,
            VkMemoryPropertyFlags properties);

    static void CreateBuffer(
            VkPhysicalDevice physicalDevice,
            VkDevice vkDevice,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);

    static void CopyBuffer(
            VkDevice vkDevice,
            VkCommandPool commandPool,
            VkQueue queue,
            VkBuffer srcBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize size);

    [[nodiscard]] static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

    static void CreateImage(
            VkPhysicalDevice physicalDevice,
            VkDevice vkDevice,
            std::uint32_t width,
            std::uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory);

    static void CreateImage2DArray(
            VkPhysicalDevice physicalDevice,
            VkDevice vkDevice,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t arrayLayers,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory,
            std::uint32_t mipLevels = 1);

    [[nodiscard]] static VkImageAspectFlags ImageAspectForFormat(VkFormat format) noexcept;

    [[nodiscard]] static VkImageView CreateImageView2DArray(
            VkDevice vkDevice,
            VkImage image,
            VkFormat format,
            std::uint32_t layerCount,
            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            std::uint32_t mipLevelCount = 1);

    [[nodiscard]] static VkSampler CreateTextureSampler(VkDevice vkDevice, float maxLod = 0.0F);
    [[nodiscard]] static VkSampler CreateSpriteSceneTextureSampler(VkDevice vkDevice);

    [[nodiscard]] static VkSampler CreateFontAtlasSampler(VkDevice vkDevice);

    static void SceneTexBarrier(
            VkCommandBuffer cmd,
            VkImage image,
            std::uint32_t layerCount,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            std::uint32_t mipLevelCount = 1);

    /** Generate mip chain for array layers [baseArrayLayer, baseArrayLayer + layerCount). */
    static void GenerateMipmapsBlit(
            VkCommandBuffer cmd,
            VkImage image,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t baseArrayLayer,
            std::uint32_t layerCount,
            std::uint32_t mipLevelCount);

    /** Copy mip0 into all lower mips with nearest filtering (avoids atlas bleed for pixel art). */
    static void GenerateMipmapsNearestFromBase(
            VkCommandBuffer cmd,
            VkImage image,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t baseArrayLayer,
            std::uint32_t layerCount,
            std::uint32_t mipLevelCount);
};

}  // namespace VulkanRendererGpu
}  // namespace Spark
