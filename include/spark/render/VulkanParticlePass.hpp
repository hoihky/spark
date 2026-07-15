#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** GPU particle billboards rendered into the HDR pass. */
class VulkanParticlePass {
public:
    void CreateGpuResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            std::uint32_t framesInFlight,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGpuResources(VkDevice device);

    void CreateGraphicsPipeline(VkDevice device, VkRenderPass hdrRenderPass);
    void DestroyGraphicsPipeline(VkDevice device);

    void Record(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const SceneRenderParams& scene,
            bool sceneParamsValid) const;

private:
    struct ParticleUniformGpu {
        float viewProj[16]{};
        float cameraPos[4]{};
        float cameraRight[4]{};
        float cameraUp[4]{};
    };

    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    Array<VkDescriptorSet> descriptorSets_;
    Array<VkBuffer> uniformBuffers_;
    Array<VkDeviceMemory> uniformBuffersMemory_;
    Array<void*> uniformBuffersMapped_;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    void* vertexMapped_ = nullptr;
    VkDeviceSize vertexCapacityBytes_ = 0;
    mutable Array<float> scratchVertices_;
};

}  // namespace Spark
