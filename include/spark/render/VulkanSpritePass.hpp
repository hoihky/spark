#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/SceneBlendMode.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"
#include "spark/render/VulkanSpriteInstanceGpu.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

struct VulkanSpriteRecordContext {
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

/** Textured sprite quads rendered into the HDR pass (GPU-instanced batches). */
class VulkanSpritePass {
public:
    void CreateGpuResources(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void DestroyGpuResources(VkDevice device);

    void CreateGraphicsPipeline(
            VkDevice device,
            VkRenderPass hdrRenderPass,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    void Record(VkCommandBuffer commandBuffer, const VulkanSpriteRecordContext& ctx) const;

    /** Resets the per-frame instance write cursor (call once before recording 2D composite). */
    void ResetInstancing(std::uint32_t frameIndex) noexcept;

    /** Reserves <c>count</c> instance slots in the shared SSBO; returns writable mapped memory. */
    [[nodiscard]] bool ReserveQuadInstances(
            std::uint32_t frameIndex,
            std::uint32_t count,
            std::uint32_t& outInstanceBase,
            VulkanSpriteInstanceGpu*& outInstances) const;

    /** Instanced indexed draw for a contiguous instance range already written to the SSBO. */
    void DrawInstancedQuads(
            VkCommandBuffer commandBuffer,
            const VulkanSpriteRecordContext& ctx,
            std::uint32_t instanceBase,
            std::uint32_t instanceCount,
            SceneBlendMode blendMode) const;

    /** Instanced draw for one or more sprites sharing blend mode (batch by texture + lighting in composite pass). */
    void DrawSpritesBatched(
            VkCommandBuffer commandBuffer,
            const VulkanSpriteRecordContext& ctx,
            const SceneRenderParams& scene,
            const std::size_t* spriteIndices,
            std::size_t spriteCount,
            SceneBlendMode blendMode) const;

    /** Single-sprite draw (one instance). */
    void DrawSprite(
            VkCommandBuffer commandBuffer,
            const VulkanSpriteRecordContext& ctx,
            const SceneSpriteDraw& sprite) const;

    [[nodiscard]] VkBuffer InstanceBuffer(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept { return pipelineLayout_; }

private:
    struct SpriteBatchPushConstants {
        std::uint32_t instanceBase = 0;
    };

    [[nodiscard]] VkPipeline PipelineForBlendMode(SceneBlendMode mode) const noexcept;
    [[nodiscard]] bool WriteInstances(
            std::uint32_t frameIndex,
            const SceneRenderParams& scene,
            const std::size_t* spriteIndices,
            std::size_t spriteCount,
            std::uint32_t& outInstanceBase) const;

    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;
    VkPipeline pipelines_[kSceneBlendModeCount]{};
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

    Array<VkBuffer> instanceBuffers_;
    Array<VkDeviceMemory> instanceMemory_;
    Array<void*> instanceMapped_;
    mutable Array<std::uint32_t> instanceWriteCursor_;
};

}  // namespace Spark
