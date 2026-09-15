#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VulkanHdrTonemapPass;
class VulkanSceneOpaqueBackground;

/**
 * Interrupts the HDR opaque pass, copies color (+ optional depth) into per-flight scratch
 * images (descriptor bindings **13** / **14**), and resumes the HDR pass for dependent draws
 * (water refraction, glTF transmission).
 */
class VulkanSceneOpaqueSnapshot {
public:
    [[nodiscard]] static bool IsRequired(bool hasWaterDraws, bool hasTransparentDraws) noexcept;

    static void RecordInterruptAndExport(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            VulkanHdrTonemapPass& hdrTonemapPass,
            VulkanSceneOpaqueBackground& opaqueBackground,
            VkImage hdrColorImage,
            VkImage sceneDepthImage,
            bool exportSceneDepth);

    static void RecordResumeOpaquePass(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            VulkanHdrTonemapPass& hdrTonemapPass);
};

}  // namespace Spark
