#include "spark/render/scene/VulkanSceneOpaqueSnapshot.hpp"

#include "spark/render/post/VulkanHdrTonemapPass.hpp"
#include "spark/render/scene/VulkanSceneOpaqueBackground.hpp"

namespace Spark {

bool VulkanSceneOpaqueSnapshot::IsRequired(const bool hasWaterDraws, const bool hasTransparentDraws) noexcept {
    return hasWaterDraws || hasTransparentDraws;
}

void VulkanSceneOpaqueSnapshot::RecordInterruptAndExport(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VkExtent2D extent,
        VulkanHdrTonemapPass& hdrTonemapPass,
        VulkanSceneOpaqueBackground& opaqueBackground,
        const VkImage hdrColorImage,
        const VkImage sceneDepthImage,
        const bool exportSceneDepth) {
    vkCmdEndRenderPass(commandBuffer);
    hdrTonemapPass.MarkColorEndedRenderPass(frameIndex);

    opaqueBackground.RecordCopyFromHdrColor(commandBuffer, frameIndex, hdrColorImage, extent);
    if (exportSceneDepth && sceneDepthImage != VK_NULL_HANDLE) {
        opaqueBackground.RecordCopyFromSceneDepth(commandBuffer, frameIndex, sceneDepthImage, extent);
    }
}

void VulkanSceneOpaqueSnapshot::RecordResumeOpaquePass(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VkExtent2D extent,
        VulkanHdrTonemapPass& hdrTonemapPass) {
    hdrTonemapPass.BeginColorAttachmentBarrierIfNeeded(commandBuffer, frameIndex);
    hdrTonemapPass.BeginHdrResumeRenderPass(commandBuffer, frameIndex, extent);
}

}  // namespace Spark
