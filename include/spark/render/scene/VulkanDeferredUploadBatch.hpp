#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VulkanSceneHdrTextureUploader;
class VulkanSceneTextureUploader;
class VulkanScreenUiPass;

/**
 * Batches CPU staging for scene LDR/HDR texture arrays and UI font atlas uploads, then records GPU
 * transfers at the start of the primary command buffer (before shadow/scene passes).
 */
class VulkanDeferredUploadBatch {
public:
    void Prepare(
            VulkanSceneTextureUploader& sceneTextures,
            VulkanSceneHdrTextureUploader& sceneHdrTextures,
            VulkanScreenUiPass& screenUi,
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            const SceneRenderParams& scene,
            bool sceneParamsValid,
            std::uint64_t frameCounter,
            std::uint32_t maxFramesInFlight);

    void Record(
            VkCommandBuffer commandBuffer,
            VkDevice device,
            VulkanSceneTextureUploader& sceneTextures,
            VulkanSceneHdrTextureUploader& sceneHdrTextures,
            VulkanScreenUiPass& screenUi);

    /** Scene texture arrays are updated in-place; wait for in-flight frames before overwriting. */
    [[nodiscard]] bool NeedsSceneTextureGpuIdle(
            const VulkanSceneTextureUploader& sceneTextures,
            const VulkanSceneHdrTextureUploader& sceneHdrTextures,
            const SceneRenderParams& scene,
            bool sceneParamsValid) const noexcept;

    /** UI sprite atlas is replaced wholesale; wait for in-flight frames before re-upload. */
    [[nodiscard]] bool NeedsUiTextureGpuIdle(
            const VulkanScreenUiPass& screenUi,
            const SceneRenderParams& scene) const noexcept;
};

}  // namespace Spark
