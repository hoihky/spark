#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * GPU 2D texture array for scene PBR materials (UNORM storage; albedo decoded sRGB in shaders).
 * Tracks content fingerprints so in-place CPU texture edits trigger re-upload.
 */
class VulkanSceneTextureUploader {
public:
    static constexpr std::uint32_t kLayerSize = 512;
    static constexpr std::uint32_t kLayerCount = 16;

    void CreateResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkCommandPool commandPool,
            VkQueue graphicsQueue);
    void DestroyResources(VkDevice device);

    [[nodiscard]] bool NeedsUpload(const SceneRenderParams& scene, bool sceneParamsValid) const noexcept;
    /** CPU resample + staging memcpy; call before primary command buffer record. */
    void PrepareUploads(const SceneRenderParams& scene, bool sceneParamsValid);
    /** GPU buffer→image copies; no-op unless <c>PrepareUploads</c> marked work pending. */
    void RecordUploads(VkCommandBuffer commandBuffer);

    [[nodiscard]] VkImageView ArrayView() const noexcept { return arrayView_; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler_; }
    [[nodiscard]] VkImageLayout Layout() const noexcept { return layout_; }

private:
    void ResetUploadCache() noexcept;

    VkDevice device_ = VK_NULL_HANDLE;
    VkImage arrayImage_ = VK_NULL_HANDLE;
    VkDeviceMemory arrayMemory_ = VK_NULL_HANDLE;
    VkImageView arrayView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory_ = VK_NULL_HANDLE;
    void* stagingMapped_ = nullptr;
    VkDeviceSize stagingSize_ = 0;
    VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    std::uint64_t lastFingerprints_[kLayerCount]{};
    std::uint32_t lastUploadedCount_ = 0xffffffffu;
    bool uploadPending_ = false;
    std::uint32_t pendingUploadCount_ = 0;
    std::uint64_t pendingFingerprints_[kLayerCount]{};
};

}  // namespace Spark
