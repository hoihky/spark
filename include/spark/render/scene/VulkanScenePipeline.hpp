#pragma once

#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Lit + sky scene graphics pipelines for the HDR render pass (scene.vert / scene.frag).
 * Composed by <c>VulkanRenderer</c>; shares the scene descriptor set layout.
 */
class VulkanScenePipeline {
public:
    void CreateGraphicsPipeline(
            VkDevice device,
            VkRenderPass hdrRenderPass,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    [[nodiscard]] VkPipeline PipelineLit() const noexcept { return pipelineLit; }
    [[nodiscard]] VkPipeline PipelineLitTransparent() const noexcept { return pipelineLitTransparent; }
    [[nodiscard]] VkPipeline PipelineSky() const noexcept { return pipelineSky; }
    [[nodiscard]] VkPipelineLayout PipelineLayout() const noexcept { return pipelineLayout; }

private:
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipelineLit = VK_NULL_HANDLE;
    VkPipeline pipelineLitTransparent = VK_NULL_HANDLE;
    VkPipeline pipelineSky = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

}  // namespace Spark
