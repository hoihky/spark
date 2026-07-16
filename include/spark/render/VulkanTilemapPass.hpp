#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/SceneBlendMode.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VulkanSpritePass;
struct VulkanSpriteRecordContext;

struct VulkanTilemapRecordContext {
    const SceneRenderParams* scene = nullptr;
    bool sceneParamsValid = false;
    std::uint32_t frameIndex = 0;
    VkExtent2D extent{};
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    std::uint32_t quadFirstIndex = 0;
    std::uint32_t quadIndexCount = 0;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

/** Culled tilemap layers rendered into the HDR pass (GPU-instanced quads, shares sprite shader + instance SSBO). */
class VulkanTilemapPass {
public:
    void CreateGraphicsPipeline(
            VkDevice device,
            VkRenderPass hdrRenderPass,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    void Record(
            VkCommandBuffer commandBuffer,
            const VulkanTilemapRecordContext& ctx,
            VulkanSpritePass& instancing,
            const VulkanSpriteRecordContext& spriteCtx) const;

    /** Draws one culled tilemap layer as a single instanced quad batch (chunks if SSBO is full). */
    void DrawLayer(
            VkCommandBuffer commandBuffer,
            const VulkanTilemapRecordContext& ctx,
            const SceneTilemapDraw& layer,
            VulkanSpritePass& instancing,
            const VulkanSpriteRecordContext& spriteCtx) const;

    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept { return pipelineLayout; }

private:
    [[nodiscard]] VkPipeline PipelineForBlendMode(SceneBlendMode mode) const noexcept;

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipelines[kSceneBlendModeCount]{};
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

}  // namespace Spark
