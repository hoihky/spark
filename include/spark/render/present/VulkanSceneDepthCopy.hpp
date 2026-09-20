#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * Destination image for copying scene depth into a shader-sampled texture.
 * Layout is tracked across frames (same pattern as SSAO and opaque-background depth).
 */
struct VulkanSceneDepthCopyDst {
    VkImage image = VK_NULL_HANDLE;
    VkImageLayout* layout = nullptr;
};

/**
 * Copies the scene depth attachment into @p dst for shader sampling.
 * Reused by <c>VulkanScreenSpaceEffectsPass</c> (SSAO) and <c>VulkanSceneOpaqueBackground</c> (water SSR).
 */
void VulkanRecordCopySceneDepthToSampled(
        VkCommandBuffer commandBuffer,
        VkImage sceneDepthImage,
        VulkanSceneDepthCopyDst& dst,
        VkExtent2D extent);

}  // namespace Spark
