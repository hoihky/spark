#include "spark/render/scene/VulkanDeferredUploadBatch.hpp"

#include "spark/render/scene/VulkanSceneTextureUploader.hpp"
#include "spark/render/ui/VulkanScreenUiPass.hpp"

namespace Spark {

void VulkanDeferredUploadBatch::Prepare(
        VulkanSceneTextureUploader& sceneTextures,
        VulkanScreenUiPass& screenUi,
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const SceneRenderParams& scene,
        const bool sceneParamsValid,
        const std::uint64_t frameCounter,
        const std::uint32_t maxFramesInFlight) {
    sceneTextures.PrepareUploads(scene, sceneParamsValid);
    screenUi.PrepareFontUpload(physicalDevice, device, scene, frameCounter, maxFramesInFlight);
}

void VulkanDeferredUploadBatch::Record(
        const VkCommandBuffer commandBuffer,
        const VkDevice device,
        VulkanSceneTextureUploader& sceneTextures,
        VulkanScreenUiPass& screenUi) {
    sceneTextures.RecordUploads(commandBuffer);
    screenUi.RecordFontUpload(commandBuffer, device);
}

bool VulkanDeferredUploadBatch::NeedsSceneTextureGpuIdle(
        const VulkanSceneTextureUploader& sceneTextures,
        const SceneRenderParams& scene,
        const bool sceneParamsValid) const noexcept {
    return sceneTextures.NeedsUpload(scene, sceneParamsValid);
}

}  // namespace Spark
