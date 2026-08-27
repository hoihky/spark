#include "spark/render/scene/VulkanIblBrdfLut.hpp"

#include "spark/render/core/VulkanRendererGpu.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Spark {

namespace {

constexpr std::uint32_t kLutSize = 512;

float RadicalInverseVdC(std::uint32_t bits) noexcept {
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return static_cast<float>(bits) * 2.3283064365386963e-10F;
}

void Hammersley(std::uint32_t i, std::uint32_t n, float& xi, float& yi) noexcept {
    xi = static_cast<float>(i) / static_cast<float>(n);
    yi = RadicalInverseVdC(i);
}

float GeometrySchlickGGX(float nDotX, float roughness) noexcept {
    const float r = roughness + 1.0F;
    const float k = (r * r) / 8.0F;
    return nDotX / (nDotX * (1.0F - k) + k);
}

float GeometrySmith(float nDotV, float nDotL, float roughness) noexcept {
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

void ImportanceSampleGGX(float xiX, float xiY, float roughness, float& hx, float& hy, float& hz) noexcept {
    const float a = roughness * roughness;
    const float phi = 2.0F * 3.14159265359F * xiX;
    const float cosTheta = std::sqrt((1.0F - xiY) / (1.0F + (a * a - 1.0F) * xiY));
    const float sinTheta = std::sqrt(std::max(1.0F - cosTheta * cosTheta, 0.0F));
    hx = std::cos(phi) * sinTheta;
    hy = std::sin(phi) * sinTheta;
    hz = cosTheta;
}

void IntegrateBrdf(const float roughness, const float nDotV, float& outA, float& outB) noexcept {
    const float nDotVClamped = std::max(nDotV, 1.0e-4F);
    const float vx = std::sqrt(std::max(1.0F - nDotVClamped * nDotVClamped, 0.0F));
    const float vy = 0.0F;
    const float vz = nDotVClamped;

    float a = 0.0F;
    float b = 0.0F;
    constexpr std::uint32_t kSampleCount = 1024U;
    for (std::uint32_t i = 0; i < kSampleCount; ++i) {
        float xiX = 0.0F;
        float xiY = 0.0F;
        Hammersley(i, kSampleCount, xiX, xiY);
        float hx = 0.0F;
        float hy = 0.0F;
        float hz = 0.0F;
        ImportanceSampleGGX(xiX, xiY, roughness, hx, hy, hz);
        const float lx = 2.0F * (vx * hx + vy * hy + vz * hz) * hx - vx;
        const float ly = 2.0F * (vx * hx + vy * hy + vz * hz) * hy - vy;
        const float lz = 2.0F * (vx * hx + vy * hy + vz * hz) * hz - vz;
        const float nDotL = std::max(lz, 0.0F);
        const float nDotH = std::max(hz, 0.0F);
        const float vDotH = std::max(vx * hx + vy * hy + vz * hz, 0.0F);
        if (nDotL > 0.0F) {
            const float g = GeometrySmith(nDotVClamped, nDotL, roughness);
            const float gVis = (g * vDotH) / (nDotH * nDotVClamped + 1.0e-4F);
            const float fc = std::pow(1.0F - vDotH, 5.0F);
            a += (1.0F - fc) * gVis;
            b += fc * gVis;
        }
    }
    outA = a / static_cast<float>(kSampleCount);
    outB = b / static_cast<float>(kSampleCount);
}

}  // namespace

void VulkanIblBrdfLut::Create(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue) {
    std::vector<std::uint16_t> pixels(static_cast<std::size_t>(kLutSize) * static_cast<std::size_t>(kLutSize) * 2U);
    for (std::uint32_t y = 0; y < kLutSize; ++y) {
        const float roughness = (static_cast<float>(y) + 0.5F) / static_cast<float>(kLutSize);
        for (std::uint32_t x = 0; x < kLutSize; ++x) {
            const float nDotV = (static_cast<float>(x) + 0.5F) / static_cast<float>(kLutSize);
            float scale = 0.0F;
            float bias = 0.0F;
            IntegrateBrdf(roughness, nDotV, scale, bias);
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kLutSize) + static_cast<std::size_t>(x)) * 2U;
            pixels[i + 0] = static_cast<std::uint16_t>(std::clamp(scale, 0.0F, 1.0F) * 65535.0F);
            pixels[i + 1] = static_cast<std::uint16_t>(std::clamp(bias, 0.0F, 1.0F) * 65535.0F);
        }
    }

    VulkanRendererGpu::CreateImage(
            physicalDevice,
            device,
            kLutSize,
            kLutSize,
            VK_FORMAT_R16G16_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            memory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R16G16_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("VulkanIblBrdfLut: vkCreateImageView failed");
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanIblBrdfLut: vkCreateSampler failed");
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixels.size() * sizeof(std::uint16_t));
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);
    void* mapped = nullptr;
    if (vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanIblBrdfLut: vkMapMemory failed");
    }
    std::memcpy(mapped, pixels.data(), static_cast<std::size_t>(imageSize));
    vkUnmapMemory(device, stagingMemory);

    VulkanRendererGpu::RunOneTimeCommands(device, commandPool, graphicsQueue, [&](const VkCommandBuffer commandBuffer) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {kLutSize, kLutSize, 1};
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void VulkanIblBrdfLut::Destroy(const VkDevice device) noexcept {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

}  // namespace Spark
