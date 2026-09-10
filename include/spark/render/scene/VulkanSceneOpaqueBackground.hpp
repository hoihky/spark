#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * Per-flight copies of the HDR opaque color buffer (binding **13**) and scene depth (binding **14**)
 * for transmission and future water refraction.
 */
class VulkanSceneOpaqueBackground {
public:
    struct FlightTarget {
        VkImage colorImage = VK_NULL_HANDLE;
        VkDeviceMemory colorMemory = VK_NULL_HANDLE;
        VkImageView colorView = VK_NULL_HANDLE;
        VkImageLayout colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void Recreate(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkExtent2D extent,
            VkFormat sceneDepthFormat,
            std::uint32_t framesInFlight);
    /** Clears scratch images and leaves them in SHADER_READ_ONLY for descriptor binding. */
    void InitializeLayouts(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent2D extent);
    void Destroy(VkDevice device) noexcept;

    /** Copies HDR color (post-opaque + sky) into the scratch texture. Call after ending the HDR render pass. */
    void RecordCopyFromHdrColor(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkImage hdrColorImage,
            VkExtent2D extent);

    /** Copies scene depth attachment into a sampled depth image (hardware depth; linearize in shader). */
    void RecordCopyFromSceneDepth(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            const VkImage sceneDepthImage,
            VkExtent2D extent);

    [[nodiscard]] VkImageView ColorView(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkImageView DepthView(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkSampler ColorSampler() const noexcept { return colorSampler; }
    [[nodiscard]] VkSampler DepthSampler() const noexcept { return depthSampler; }
    /** @deprecated Use <c>ColorView</c>. */
    [[nodiscard]] VkImageView View(std::uint32_t frameIndex) const noexcept { return ColorView(frameIndex); }
    /** @deprecated Use <c>ColorSampler</c>. */
    [[nodiscard]] VkSampler Sampler() const noexcept { return ColorSampler(); }
    [[nodiscard]] bool HasFlight(std::uint32_t frameIndex) const noexcept {
        return frameIndex < flights.GetSize() && flights[frameIndex].colorView != VK_NULL_HANDLE;
    }

private:
    Array<FlightTarget> flights;
    VkSampler colorSampler = VK_NULL_HANDLE;
    VkSampler depthSampler = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};

}  // namespace Spark
