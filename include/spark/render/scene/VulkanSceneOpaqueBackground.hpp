#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** Per-flight copy of the HDR opaque color buffer for screen-space transmission. */
class VulkanSceneOpaqueBackground {
public:
    struct FlightTarget {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void Recreate(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkExtent2D extent,
            std::uint32_t framesInFlight);
    /** Clears scratch images to black and leaves them in SHADER_READ_ONLY for descriptor binding. */
    void InitializeLayouts(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent2D extent);
    void Destroy(VkDevice device) noexcept;

    /** Copies HDR color (post-opaque) into the scratch texture. Call after ending the HDR render pass. */
    void RecordCopyFromHdrColor(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkImage hdrColorImage,
            VkExtent2D extent);

    [[nodiscard]] VkImageView View(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler; }
    [[nodiscard]] bool HasFlight(std::uint32_t frameIndex) const noexcept {
        return frameIndex < flights.GetSize() && flights[frameIndex].view != VK_NULL_HANDLE;
    }

private:
    Array<FlightTarget> flights;
    VkSampler sampler = VK_NULL_HANDLE;
};

}  // namespace Spark
