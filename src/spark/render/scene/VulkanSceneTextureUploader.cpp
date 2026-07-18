#include "spark/render/scene/VulkanSceneTextureUploader.hpp"

#include "spark/memory/SharedPtr.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/scene/Texture2D.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Spark {

void VulkanSceneTextureUploader::ResetUploadCache() noexcept {
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = 0;
    }
    lastUploadedCount = 0xffffffffu;
}

void VulkanSceneTextureUploader::CreateResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue) {
    DestroyResources(device);
    this->device = device;

    constexpr VkFormat kTexFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VulkanRendererGpu::CreateImage2DArray(
            physicalDevice,
            device,
            kLayerSize,
            kLayerSize,
            kLayerCount,
            kTexFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            arrayImage,
            arrayMemory);
    arrayView = VulkanRendererGpu::CreateImageView2DArray(device, arrayImage, kTexFormat, kLayerCount);
    sampler = VulkanRendererGpu::CreateTextureSampler(device);

    const VkDeviceSize layerPitch =
            static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 4;
    stagingSize = layerPitch * static_cast<VkDeviceSize>(kLayerCount);
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
                cb, arrayImage, kLayerCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
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
        }
        VulkanRendererGpu::SceneTexBarrier(
                cb,
                arrayImage,
                kLayerCount,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    const std::size_t n = std::min(texCount, static_cast<std::size_t>(kLayerCount));
    const std::uint32_t n32 = static_cast<std::uint32_t>(n);
    if (lastUploadedCount != n32) {
        return true;
    }
    for (std::size_t i = 0; i < n; ++i) {
        const SharedPtr<Texture2D>& tex = scene.sceneTextures[i];
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
    const std::size_t n = std::min(texCount, static_cast<std::size_t>(kLayerCount));
    const std::uint32_t n32 = static_cast<std::uint32_t>(n);

    const VkDeviceSize layerPitch =
            static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 4;
    auto* const base = static_cast<std::uint8_t*>(stagingMapped);
    Array<std::uint8_t> resampled;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        std::uint8_t* dst = base + static_cast<std::size_t>(layerPitch) * i;
        if (i < n32) {
            const SharedPtr<Texture2D>& tex = scene.sceneTextures[i];
            if (tex) {
                tex->ResampleNearest(kLayerSize, kLayerSize, resampled);
                if (resampled.GetSize() >= static_cast<std::size_t>(layerPitch)) {
                    std::memcpy(dst, resampled.GetData(), static_cast<std::size_t>(layerPitch));
                } else {
                    std::memset(dst, 255, static_cast<std::size_t>(layerPitch));
                }
            } else {
                std::memset(dst, 255, static_cast<std::size_t>(layerPitch));
            }
        } else {
            std::memset(dst, 255, static_cast<std::size_t>(layerPitch));
        }
    }

    pendingUploadCount = n32;
    for (std::uint32_t i = 0; i < n32; ++i) {
        const SharedPtr<Texture2D>& tex = scene.sceneTextures[i];
        pendingFingerprints[i] = tex ? tex->GetContentFingerprint() : 0;
    }
    for (std::uint32_t i = n32; i < kLayerCount; ++i) {
        pendingFingerprints[i] = 0;
    }
    uploadPending = true;
}

void VulkanSceneTextureUploader::RecordUploads(const VkCommandBuffer commandBuffer) {
    if (!uploadPending || commandBuffer == VK_NULL_HANDLE || stagingMapped == nullptr ||
        arrayImage == VK_NULL_HANDLE) {
        return;
    }

    const VkDeviceSize layerPitch =
            static_cast<VkDeviceSize>(kLayerSize) * static_cast<VkDeviceSize>(kLayerSize) * 4;

    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer, arrayImage, kLayerCount, layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        VkBufferImageCopy region{};
        region.bufferOffset = layerPitch * static_cast<VkDeviceSize>(i);
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {kLayerSize, kLayerSize, 1};
        vkCmdCopyBufferToImage(
                commandBuffer, stagingBuffer, arrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    VulkanRendererGpu::SceneTexBarrier(
            commandBuffer,
            arrayImage,
            kLayerCount,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    lastUploadedCount = pendingUploadCount;
    for (std::uint32_t i = 0; i < kLayerCount; ++i) {
        lastFingerprints[i] = pendingFingerprints[i];
    }
    uploadPending = false;
}

}  // namespace Spark
