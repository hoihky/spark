#include "spark/render/scene/VulkanSceneTextureUploader.hpp"

#include "spark/memory/SharedPtr.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/gpu/VulkanTextureFormatSupport.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/TextureBlockCompressor.hpp"
#include "spark/scene/TextureFormat.hpp"
#include "spark/scene/TextureMipChain.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

constexpr std::uint32_t kMaxMipLevels = 16;

[[nodiscard]] bool UsesBlockCompression(SceneTextureArrayMode mode) noexcept {
    return mode == SceneTextureArrayMode::Bc7Mipped || mode == SceneTextureArrayMode::Astc4x4Mipped;
}

}  // namespace

void VulkanSceneTextureUploader::ResetUploadCache() noexcept {
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = 0;
    }
    lastUploadedCount = 0xffffffffu;
}

VkDeviceSize VulkanSceneTextureUploader::LayerStagingPitch() const noexcept {
    if (UsesBlockCompression(arrayMode)) {
        return static_cast<VkDeviceSize>(
                CompressedMipChainByteSize(pixelFormat, kLayerSize, kLayerSize));
    }
    return static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 4;
}

VkDeviceSize VulkanSceneTextureUploader::StagingSizeForMode() const noexcept {
    return LayerStagingPitch() * static_cast<VkDeviceSize>(kLayerCount);
}

void VulkanSceneTextureUploader::CreateResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue) {
    DestroyResources(device);
    this->physicalDevice = physicalDevice;
    this->device = device;

    arrayMode = VulkanTextureFormatSupport::SelectSceneArrayMode(physicalDevice);
    arrayFormat = VulkanTextureFormatSupport::ToVkFormat(arrayMode);
    pixelFormat = VulkanTextureFormatSupport::ToPixelFormat(arrayMode);
    mipLevelCount = CountMipLevels(kLayerSize, kLayerSize);
    samplerMaxLod = static_cast<float>(mipLevelCount > 0 ? mipLevelCount - 1U : 0U);

    const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT;
    VulkanRendererGpu::CreateImage2DArray(
            physicalDevice,
            device,
            kLayerSize,
            kLayerSize,
            kLayerCount,
            arrayFormat,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            arrayImage,
            arrayMemory,
            mipLevelCount);
    arrayView = VulkanRendererGpu::CreateImageView2DArray(
            device, arrayImage, arrayFormat, kLayerCount, VK_IMAGE_ASPECT_COLOR_BIT, mipLevelCount);
    sampler = VulkanRendererGpu::CreateTextureSampler(device, samplerMaxLod);
    spriteSampler = VulkanRendererGpu::CreateSpriteSceneTextureSampler(device);

    stagingSize = StagingSizeForMode();
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            stagingSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);
    if (vkMapMemory(device, stagingMemory, 0, stagingSize, 0, &stagingMapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanSceneTextureUploader: map staging failed");
    }
    std::memset(stagingMapped, 255, static_cast<std::size_t>(stagingSize));

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
            if (UsesBlockCompression(arrayMode)) {
                VkDeviceSize layerOffset = layerPitch * static_cast<VkDeviceSize>(i);
                VkDeviceSize mipOffset = 0;
                for (std::uint32_t level = 0; level < mipLevelCount; ++level) {
                    const std::uint32_t mipW = MipDimension(kLayerSize, level);
                    const std::uint32_t mipH = MipDimension(kLayerSize, level);
                    const VkDeviceSize mipBytes = static_cast<VkDeviceSize>(
                            CompressedMipByteSize(pixelFormat, mipW, mipH));
                    VkBufferImageCopy region{};
                    region.bufferOffset = layerOffset + mipOffset;
                    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel = level;
                    region.imageSubresource.baseArrayLayer = i;
                    region.imageSubresource.layerCount = 1;
                    region.imageExtent = {mipW, mipH, 1};
                    vkCmdCopyBufferToImage(
                            cb, stagingBuffer, arrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                    mipOffset += mipBytes;
                }
            } else {
                VkBufferImageCopy region{};
                region.bufferOffset = layerPitch * static_cast<VkDeviceSize>(i);
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = i;
                region.imageSubresource.layerCount = 1;
                region.imageExtent = {kLayerSize, kLayerSize, 1};
                vkCmdCopyBufferToImage(
                        cb, stagingBuffer, arrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                VulkanRendererGpu::GenerateMipmapsBlit(
                        cb, arrayImage, kLayerSize, kLayerSize, i, 1, mipLevelCount);
            }
        }
        VulkanRendererGpu::SceneTexBarrier(
                cb,
                arrayImage,
                kLayerCount,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                mipLevelCount);
    });
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ResetUploadCache();
}

void VulkanSceneTextureUploader::DestroyResources(const VkDevice device) {
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
    if (spriteSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, spriteSampler, nullptr);
        spriteSampler = VK_NULL_HANDLE;
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

bool VulkanSceneTextureUploader::NeedsUpload(
        const SceneRenderParams& scene,
        const bool sceneParamsValid) const noexcept {
    if (!sceneParamsValid || stagingMapped == nullptr || arrayImage == VK_NULL_HANDLE) {
        return false;
    }
    const std::size_t texCount = scene.sceneTextures.GetSize();
    std::uint32_t maxLayer = 0;
    for (std::size_t i = 0; i < texCount && i < kLayerCount; ++i) {
        if (scene.sceneTextures[i]) {
            maxLayer = static_cast<std::uint32_t>(i);
        }
    }
    if (lastUploadedCount != maxLayer + 1U) {
        return true;
    }
    for (std::uint32_t i = 0; i <= maxLayer; ++i) {
        const SharedPtr<Texture2D>& tex =
                (static_cast<std::size_t>(i) < texCount) ? scene.sceneTextures[i] : SharedPtr<Texture2D>{};
        const std::uint64_t fp = tex ? tex->GetContentFingerprint() : 0;
        if (fp != lastFingerprints[i]) {
            return true;
        }
    }
    return false;
}

void VulkanSceneTextureUploader::PrepareUploads(const SceneRenderParams& scene, const bool sceneParamsValid) {
    uploadPending = false;
    if (!NeedsUpload(scene, sceneParamsValid)) {
        return;
    }

    const std::size_t texCount = scene.sceneTextures.GetSize();
    std::uint32_t maxLayer = 0;
    for (std::size_t i = 0; i < texCount && i < kLayerCount; ++i) {
        if (scene.sceneTextures[i]) {
            maxLayer = static_cast<std::uint32_t>(i);
        }
    }
    const std::uint32_t uploadLayerCount = maxLayer + 1U;

    const VkDeviceSize layerPitch = LayerStagingPitch();
    auto* const base = static_cast<std::uint8_t*>(stagingMapped);
    Array<std::uint8_t> resampled;
    TextureMipChain rgbaChain;
    TextureMipChain compressedChain;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        std::uint8_t* dst = base + static_cast<std::size_t>(layerPitch) * i;
        pendingNearestMip[i] = false;
        const SharedPtr<Texture2D> tex =
                (i < uploadLayerCount && static_cast<std::size_t>(i) < texCount) ? scene.sceneTextures[i]
                                                                                  : SharedPtr<Texture2D>{};
        if (tex) {
                if (tex->HasPrebuiltMipChain() && tex->GetPixelFormat() == pixelFormat) {
                    const Array<TextureMipLevel>& mips = tex->GetMipChain();
                    VkDeviceSize mipOffset = 0;
                    for (std::size_t level = 0; level < mips.GetSize() && level < kMaxMipLevels; ++level) {
                        const TextureMipLevel& mip = mips[level];
                        const VkDeviceSize mipBytes = static_cast<VkDeviceSize>(mip.GetBytes().GetSize());
                        if (mipOffset + mipBytes <= layerPitch) {
                            std::memcpy(dst + mipOffset, mip.GetBytes().GetData(), static_cast<std::size_t>(mipBytes));
                        }
                        mipOffset += mipBytes;
                    }
                } else if (UsesBlockCompression(arrayMode)) {
                    tex->ResampleBilinear(kLayerSize, kLayerSize, resampled);
                    rgbaChain.BuildFromRgba(resampled, kLayerSize, kLayerSize);
                    if (TextureBlockCompressor::Get().CompressChain(pixelFormat, rgbaChain, compressedChain)) {
                        const Array<TextureMipLevel>& compressedMips = compressedChain.GetLevels();
                        VkDeviceSize mipOffset = 0;
                        for (std::size_t level = 0; level < compressedMips.GetSize(); ++level) {
                            const TextureMipLevel& mip = compressedMips[level];
                            const VkDeviceSize mipBytes = static_cast<VkDeviceSize>(mip.GetBytes().GetSize());
                            if (mipOffset + mipBytes <= layerPitch) {
                                std::memcpy(
                                        dst + mipOffset,
                                        mip.GetBytes().GetData(),
                                        static_cast<std::size_t>(mipBytes));
                            }
                            mipOffset += mipBytes;
                        }
                    } else {
                        std::memset(dst, 255, static_cast<std::size_t>(layerPitch));
                    }
                } else {
                    tex->PrepareSceneLayerUpload(kLayerSize, resampled);
                    pendingNearestMip[i] = tex->GetSceneUploadNearest();
                    const VkDeviceSize rgbaPitch =
                            static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 4;
                    if (resampled.GetSize() >= static_cast<std::size_t>(rgbaPitch)) {
                        std::memcpy(dst, resampled.GetData(), static_cast<std::size_t>(rgbaPitch));
                    } else {
                        std::memset(dst, 255, static_cast<std::size_t>(rgbaPitch));
                    }
                }
        } else {
            std::memset(dst, 255, static_cast<std::size_t>(layerPitch));
        }
    }

    pendingUploadCount = uploadLayerCount;
    for (std::uint32_t i = 0; i < uploadLayerCount; ++i) {
        const SharedPtr<Texture2D>& uploadTex =
                (static_cast<std::size_t>(i) < texCount) ? scene.sceneTextures[i] : SharedPtr<Texture2D>{};
        pendingFingerprints[i] = uploadTex ? uploadTex->GetContentFingerprint() : 0;
    }
    for (std::uint32_t i = uploadLayerCount; i < kLayerCount; ++i) {
        pendingFingerprints[i] = 0;
    }
    uploadPending = true;
}

void VulkanSceneTextureUploader::RecordUploads(const VkCommandBuffer commandBuffer) {
    if (!uploadPending || commandBuffer == VK_NULL_HANDLE || stagingMapped == nullptr ||
        arrayImage == VK_NULL_HANDLE) {
        return;
    }

    const VkDeviceSize layerPitch = LayerStagingPitch();

    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer, arrayImage, kLayerCount, layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevelCount);

    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        const VkDeviceSize layerOffset = layerPitch * static_cast<VkDeviceSize>(i);
        if (UsesBlockCompression(arrayMode)) {
            VkDeviceSize mipOffset = 0;
            for (std::uint32_t level = 0; level < mipLevelCount; ++level) {
                const std::uint32_t mipW = MipDimension(kLayerSize, level);
                const std::uint32_t mipH = MipDimension(kLayerSize, level);
                const VkDeviceSize mipBytes =
                        static_cast<VkDeviceSize>(CompressedMipByteSize(pixelFormat, mipW, mipH));
                VkBufferImageCopy region{};
                region.bufferOffset = layerOffset + mipOffset;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = level;
                region.imageSubresource.baseArrayLayer = i;
                region.imageSubresource.layerCount = 1;
                region.imageExtent = {mipW, mipH, 1};
                vkCmdCopyBufferToImage(
                        commandBuffer,
                        stagingBuffer,
                        arrayImage,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &region);
                mipOffset += mipBytes;
            }
        } else {
            VkBufferImageCopy region{};
            region.bufferOffset = layerOffset;
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
            if (pendingNearestMip[i]) {
                // Sprites sample mip 0 only; skip mip generation to avoid atlas bleed in unused levels.
            } else {
                VulkanRendererGpu::GenerateMipmapsBlit(
                        commandBuffer, arrayImage, kLayerSize, kLayerSize, i, 1, mipLevelCount);
            }
        }
    }

    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer,
            arrayImage,
            kLayerCount,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            mipLevelCount);
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    lastUploadedCount = pendingUploadCount;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = pendingFingerprints[i];
    }
    uploadPending = false;
}

}  // namespace Spark
