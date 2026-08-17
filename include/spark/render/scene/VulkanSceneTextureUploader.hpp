#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/TextureFormat.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * GPU 2D texture array for scene PBR materials.
 * Supports mipmapped RGBA8, BC7, or ASTC 4x4 encodings depending on device capabilities.
 */
class VulkanSceneTextureUploader {
public:
    static constexpr std::uint32_t kLayerSize = 1024;
    static constexpr std::uint32_t kLayerCount = 32;

    void CreateResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkCommandPool commandPool,
            VkQueue graphicsQueue);
    void DestroyResources(VkDevice device);

    [[nodiscard]] bool NeedsUpload(const SceneRenderParams& scene, bool sceneParamsValid) const noexcept;
    /** CPU resample/compress + staging memcpy; call before primary command buffer record. */
    void PrepareUploads(const SceneRenderParams& scene, bool sceneParamsValid);
    /** GPU buffer→image copies (+ optional mip blits); no-op unless <c>PrepareUploads</c> marked work pending. */
    void RecordUploads(VkCommandBuffer commandBuffer);

    [[nodiscard]] VkImageView ArrayView() const noexcept { return arrayView; }
    [[nodiscard]] VkSampler Sampler() const noexcept { return sampler; }
    [[nodiscard]] VkSampler SpriteSampler() const noexcept { return spriteSampler; }
    [[nodiscard]] VkImageLayout Layout() const noexcept { return layout; }
    [[nodiscard]] SceneTextureArrayMode ArrayMode() const noexcept { return arrayMode; }

private:
    void ResetUploadCache() noexcept;
    [[nodiscard]] VkDeviceSize LayerStagingPitch() const noexcept;
    [[nodiscard]] VkDeviceSize StagingSizeForMode() const noexcept;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    SceneTextureArrayMode arrayMode = SceneTextureArrayMode::Rgba8Mipped;
    VkFormat arrayFormat = VK_FORMAT_R8G8B8A8_UNORM;
    TexturePixelFormat pixelFormat = TexturePixelFormat::Rgba8Unorm;
    std::uint32_t mipLevelCount = 1;
    float samplerMaxLod = 0.0F;
    VkImage arrayImage = VK_NULL_HANDLE;
    VkDeviceMemory arrayMemory = VK_NULL_HANDLE;
    VkImageView arrayView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkSampler spriteSampler = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    void* stagingMapped = nullptr;
    VkDeviceSize stagingSize = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::uint64_t lastFingerprints[kLayerCount]{};
    std::uint32_t lastUploadedCount = 0xffffffffu;
    bool uploadPending = false;
    std::uint32_t pendingUploadCount = 0;
    std::uint64_t pendingFingerprints[kLayerCount]{};
    bool pendingNearestMip[kLayerCount]{};
};

}  // namespace Spark
