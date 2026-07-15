#pragma once

#include "spark/render/VulkanSpvShaderLoader.hpp"

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

    [[nodiscard]] VkPipeline PipelineLit() const noexcept { return pipelineLit_; }
    [[nodiscard]] VkPipeline PipelineLitTransparent() const noexcept { return pipelineLitTransparent_; }
    [[nodiscard]] VkPipeline PipelineSky() const noexcept { return pipelineSky_; }
    [[nodiscard]] VkPipelineLayout PipelineLayout() const noexcept { return pipelineLayout_; }

private:
    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;
    VkPipeline pipelineLit_ = VK_NULL_HANDLE;
    VkPipeline pipelineLitTransparent_ = VK_NULL_HANDLE;
    VkPipeline pipelineSky_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
};

}  // namespace Spark
