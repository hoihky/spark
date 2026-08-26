#include "spark/render/scene/VulkanSceneHdrTextureUploader.hpp"

#include "spark/memory/SharedPtr.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/texture/TextureFormat.hpp"
#include "spark/scene/texture/TextureHalfFloat.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Spark {

void VulkanSceneHdrTextureUploader::ResetUploadCache() noexcept {
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = 0;
        lastUploadedTextures[i] = nullptr;
        pendingLayerDirty[i] = false;
        pendingTextures[i] = nullptr;
    }
    lastUploadedCount = 0;
}

VkDeviceSize VulkanSceneHdrTextureUploader::LayerStagingPitch() noexcept {
    return static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 8U;
}

void VulkanSceneHdrTextureUploader::CreateResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue) {
    DestroyResources(device);
    this->physicalDevice = physicalDevice;
    this->device = device;

    constexpr VkFormat kFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    mipLevelCount = CountMipLevels(kLayerSize, kLayerSize);
    samplerMaxLod = static_cast<float>(mipLevelCount > 0U ? mipLevelCount - 1U : 0U);
    const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT;
    VulkanRendererGpu::CreateImage2DArray(
            physicalDevice,
            device,
            kLayerSize,
            kLayerSize,
            kLayerCount,
            kFormat,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            arrayImage,
            arrayMemory,
            mipLevelCount);
    arrayView = VulkanRendererGpu::CreateImageView2DArray(
            device, arrayImage, kFormat, kLayerCount, VK_IMAGE_ASPECT_COLOR_BIT, mipLevelCount);
    sampler = VulkanRendererGpu::CreateEquirectSceneTextureSampler(device, samplerMaxLod);

    stagingSize = LayerStagingPitch() * static_cast<VkDeviceSize>(kLayerCount);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            stagingSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);
    if (vkMapMemory(device, stagingMemory, 0, stagingSize, 0, &stagingMapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanSceneHdrTextureUploader: map staging failed");
    }
    std::memset(stagingMapped, 0, static_cast<std::size_t>(stagingSize));

    VulkanRendererGpu::RunOneTimeCommands(device, commandPool, graphicsQueue, [&](const VkCommandBuffer cb) {
        VulkanRendererGpu::SceneTexBarrier(
                cb,
                arrayImage,
                kLayerCount,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipLevelCount);
        const VkDeviceSize layerPitch = LayerStagingPitch();
        for (std::uint32_t i = 0; i < kLayerCount; ++i) {
            VkBufferImageCopy region{};
            region.bufferOffset = layerPitch * static_cast<VkDeviceSize>(i);
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = i;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {kLayerSize, kLayerSize, 1};
            vkCmdCopyBufferToImage(
                    cb, stagingBuffer, arrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            VulkanRendererGpu::SceneTexBarrierRegion(
                    cb,
                    arrayImage,
                    i,
                    1,
                    0,
                    mipLevelCount,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ResetUploadCache();
}

void VulkanSceneHdrTextureUploader::DestroyResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (stagingMapped != nullptr) {
        vkUnmapMemory(device, stagingMemory);
        stagingMapped = nullptr;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    if (stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, stagingMemory, nullptr);
        stagingMemory = VK_NULL_HANDLE;
    }
    stagingSize = 0;
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (arrayView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, arrayView, nullptr);
        arrayView = VK_NULL_HANDLE;
    }
    if (arrayImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, arrayImage, nullptr);
        arrayImage = VK_NULL_HANDLE;
    }
    if (arrayMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, arrayMemory, nullptr);
        arrayMemory = VK_NULL_HANDLE;
    }
    layout = VK_IMAGE_LAYOUT_UNDEFINED;
    this->device = VK_NULL_HANDLE;
    this->physicalDevice = VK_NULL_HANDLE;
    uploadPending = false;
    ResetUploadCache();
}

bool VulkanSceneHdrTextureUploader::NeedsUpload(
        const SceneRenderParams& scene,
        const bool sceneParamsValid) const noexcept {
    if (!sceneParamsValid || stagingMapped == nullptr || arrayImage == VK_NULL_HANDLE) {
        return false;
    }
    const std::size_t texCount = scene.sceneHdrTextures.GetSize();
    std::int32_t maxLayerIndex = -1;
    for (std::size_t i = 0; i < texCount && i < kLayerCount; ++i) {
        if (scene.sceneHdrTextures[i]) {
            maxLayerIndex = static_cast<std::int32_t>(i);
        }
    }
    if (maxLayerIndex < 0) {
        return false;
    }
    const std::uint32_t maxLayer = static_cast<std::uint32_t>(maxLayerIndex);
    if (maxLayer + 1U > lastUploadedCount) {
        return true;
    }
    for (std::uint32_t i = 0; i <= maxLayer; ++i) {
        const SharedPtr<Texture2D>& tex =
                (static_cast<std::size_t>(i) < texCount) ? scene.sceneHdrTextures[i] : SharedPtr<Texture2D>{};
        if (!tex) {
            continue;
        }
        if (tex->GetContentFingerprint() != lastFingerprints[i] ||
            tex.Get() != lastUploadedTextures[i]) {
            return true;
        }
    }
    return false;
}

void VulkanSceneHdrTextureUploader::PrepareUploads(const SceneRenderParams& scene, const bool sceneParamsValid) {
    uploadPending = false;
    if (!NeedsUpload(scene, sceneParamsValid)) {
        return;
    }

    const std::size_t texCount = scene.sceneHdrTextures.GetSize();
    std::int32_t maxLayerIndex = -1;
    for (std::size_t i = 0; i < texCount && i < kLayerCount; ++i) {
        if (scene.sceneHdrTextures[i]) {
            maxLayerIndex = static_cast<std::int32_t>(i);
        }
    }
    if (maxLayerIndex < 0) {
        return;
    }
    const std::uint32_t maxLayer = static_cast<std::uint32_t>(maxLayerIndex);
    const std::uint32_t uploadLayerCount =
            std::min(std::max(maxLayer + 1U, lastUploadedCount), kLayerCount);

    const VkDeviceSize layerPitch = LayerStagingPitch();
    auto* const base = static_cast<std::uint8_t*>(stagingMapped);
    Array<float> resampled;
    Array<std::uint16_t> packed;
    bool anyLayerDirty = maxLayer + 1U > lastUploadedCount;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        pendingLayerDirty[i] = false;
        pendingFingerprints[i] = lastFingerprints[i];
        pendingTextures[i] = lastUploadedTextures[i];

        const SharedPtr<Texture2D> tex =
                (static_cast<std::size_t>(i) < texCount) ? scene.sceneHdrTextures[i] : SharedPtr<Texture2D>{};
        if (!tex) {
            continue;
        }

        const std::uint64_t fp = tex->GetContentFingerprint();
        const bool layerDirty =
                i >= lastUploadedCount || fp != lastFingerprints[i] || tex.Get() != lastUploadedTextures[i];
        if (!layerDirty) {
            continue;
        }

        pendingLayerDirty[i] = true;
        anyLayerDirty = true;
        pendingFingerprints[i] = fp;
        pendingTextures[i] = tex.Get();

        tex->PrepareSceneLayerUploadFloat(kLayerSize, resampled);
        const std::size_t pixelCount = static_cast<std::size_t>(kLayerSize) * static_cast<std::size_t>(kLayerSize);
        packed.Resize(pixelCount * 4U);
        TextureHalfFloat::PackRgba16Float(resampled.GetData(), packed.GetData(), pixelCount);

        std::uint8_t* const dst = base + static_cast<std::size_t>(layerPitch) * i;
        const VkDeviceSize packedBytes = static_cast<VkDeviceSize>(packed.GetSize() * sizeof(std::uint16_t));
        if (packedBytes <= layerPitch) {
            std::memcpy(dst, packed.GetData(), static_cast<std::size_t>(packedBytes));
        } else {
            std::memset(dst, 0, static_cast<std::size_t>(layerPitch));
        }
    }

    if (!anyLayerDirty) {
        return;
    }

    pendingUploadCount = uploadLayerCount;
    uploadPending = true;
}

void VulkanSceneHdrTextureUploader::RecordUploads(const VkCommandBuffer commandBuffer) {
    if (!uploadPending || commandBuffer == VK_NULL_HANDLE || stagingMapped == nullptr ||
        arrayImage == VK_NULL_HANDLE) {
        return;
    }

    const VkDeviceSize layerPitch = LayerStagingPitch();
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        if (!pendingLayerDirty[i]) {
            continue;
        }

        VulkanRendererGpu::SceneTexBarrierRegion(
                commandBuffer,
                arrayImage,
                i,
                1,
                0,
                1,
                layout,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region{};
        region.bufferOffset = layerPitch * static_cast<VkDeviceSize>(i);
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {kLayerSize, kLayerSize, 1};
        vkCmdCopyBufferToImage(
                commandBuffer,
                stagingBuffer,
                arrayImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);
        VulkanRendererGpu::GenerateMipmapsBlit(
                commandBuffer, arrayImage, kLayerSize, kLayerSize, i, 1, mipLevelCount);
    }

    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    lastUploadedCount = pendingUploadCount;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = pendingFingerprints[i];
        lastUploadedTextures[i] = pendingTextures[i];
    }
    uploadPending = false;
}

}  // namespace Spark
