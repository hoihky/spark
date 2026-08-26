#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class Texture2D;

/**
 * GPU 2D texture array for linear HDR scene textures (R16G16B16A16_SFLOAT).
 * Separate from the LDR RGBA8/BC7/ASTC array because Vulkan array images require a single format.
 */
class VulkanSceneHdrTextureUploader {
public:
    static constexpr std::uint32_t kLayerSize = 1024;
    static constexpr std::uint32_t kLayerCount = 8;

    void CreateResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkCommandPool commandPool,
            VkQueue graphicsQueue);
    void DestroyResources(VkDevice device);

    [[nodiscard]] bool NeedsUpload(const SceneRenderParams& scene, bool sceneParamsValid) const noexcept;
    void PrepareUploads(const SceneRenderParams& scene, bool sceneParamsValid);
    void RecordUploads(VkCommandBuffer commandBuffer);

    [[nodiscard]] VkImageView ArrayView() const noexcept { return arrayView; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler; }
    [[nodiscard]] VkImageLayout Layout() const noexcept { return layout; }

private:
    void ResetUploadCache() noexcept;
    [[nodiscard]] static VkDeviceSize LayerStagingPitch() noexcept;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkImage arrayImage = VK_NULL_HANDLE;
    VkDeviceMemory arrayMemory = VK_NULL_HANDLE;
    VkImageView arrayView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    void* stagingMapped = nullptr;
    VkDeviceSize stagingSize = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    const Texture2D* lastUploadedTextures[kLayerCount]{};
    std::uint64_t lastFingerprints[kLayerCount]{};
    std::uint32_t lastUploadedCount = 0;
    std::uint32_t mipLevelCount = 1;
    float samplerMaxLod = 0.0F;
    bool uploadPending = false;
    std::uint32_t pendingUploadCount = 0;
    std::uint64_t pendingFingerprints[kLayerCount]{};
    const Texture2D* pendingTextures[kLayerCount]{};
    bool pendingLayerDirty[kLayerCount]{};
};

}  // namespace Spark
