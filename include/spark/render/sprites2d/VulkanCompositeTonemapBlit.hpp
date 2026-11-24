#pragma once

#include "spark/render/post/VulkanHdrTonemapPass.hpp"
#include "spark/render/scene/VulkanOffscreenRenderTarget.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** Tonemap HDR composite captures into a scratch RGBA8 image for UI atlas copy. */
class VulkanCompositeTonemapBlit final {
public:
    void BindDevice(VkPhysicalDevice physicalDeviceIn, VkDevice deviceIn) noexcept;
    void Create(VkDevice device);
    void Destroy(VkDevice device) noexcept;

    void EnsureScratch(VkExtent2D extent);
    void RecordTonemap(
            VkCommandBuffer commandBuffer,
            VulkanHdrTonemapPass& tonemapPass,
            std::uint32_t flightIndex,
            float exposure) noexcept;

    [[nodiscard]] VkRenderPass TonemapOutputRenderPass() const noexcept { return tonemapOutputRenderPass; }
    [[nodiscard]] VkImage ScratchColorImage() const noexcept { return scratchColorImage; }
    [[nodiscard]] VkExtent2D ScratchExtent() const noexcept { return scratchExtent; }
    [[nodiscard]] VkImageLayout ScratchColorLayout() const noexcept { return scratchColorLayout; }

private:
    void DestroyScratch(VkDevice device) noexcept;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass tonemapOutputRenderPass = VK_NULL_HANDLE;
    VkImage scratchColorImage = VK_NULL_HANDLE;
    VkDeviceMemory scratchColorMemory = VK_NULL_HANDLE;
    VkImageView scratchColorView = VK_NULL_HANDLE;
    VkFramebuffer scratchFramebuffer = VK_NULL_HANDLE;
    VkExtent2D scratchExtent{};
    VkImageLayout scratchColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

}  // namespace Spark