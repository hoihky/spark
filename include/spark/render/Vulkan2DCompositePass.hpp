#pragma once

#include "spark/render/VulkanSpritePass.hpp"
#include "spark/render/VulkanTilemapPass.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Records tilemaps and sprites in correct compositing order: blend pass first
 * (multiply → opaque → alpha → screen → additive), then sorting layer and <c>sortOrder</c>.
 */
class Vulkan2DCompositePass {
public:
    void Record(
            VkCommandBuffer commandBuffer,
            const VulkanTilemapPass& tilemapPass,
            VulkanSpritePass& spritePass,
            const VulkanTilemapRecordContext& tilemapCtx,
            const VulkanSpriteRecordContext& spriteCtx) const;
};

}  // namespace Spark
