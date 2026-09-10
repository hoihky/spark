#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanCustomMeshPool.hpp"
#include "spark/render/scene/VulkanSceneMeshDraw.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VulkanSpvShaderLoader;

struct VulkanWaterRecordContext {
    const SceneRenderParams* scene = nullptr;
    bool sceneParamsValid = false;
    std::uint32_t frameIndex = 0;
    VkExtent2D extent{};
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    SceneMeshDrawBindings meshBindings{};
};

/**
 * Dedicated Gerstner water pass after opaque (+ sky), before generic transparent meshes.
 * Uses scene descriptor set (UBO, IBL, shadow maps) plus water push constants (wave array).
 */
class VulkanWaterPass {
public:
    void CreateGraphicsPipeline(
            VkDevice device,
            VkRenderPass hdrRenderPass,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device) noexcept;

    void Record(
            VkCommandBuffer commandBuffer,
            const VulkanWaterRecordContext& ctx,
            const Array<CustomMeshGpuSlice>& waterCustomPacked) const;

    [[nodiscard]] VkPipeline Pipeline() const noexcept { return pipeline; }
    [[nodiscard]] VkPipelineLayout PipelineLayout() const noexcept { return pipelineLayout; }

private:
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

}  // namespace Spark
