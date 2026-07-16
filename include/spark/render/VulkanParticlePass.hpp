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

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> descriptorSets;
    Array<VkBuffer> uniformBuffers;
    Array<VkDeviceMemory> uniformBuffersMemory;
    Array<void*> uniformBuffersMapped;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    void* vertexMapped = nullptr;
    VkDeviceSize vertexCapacityBytes = 0;
    mutable Array<float> scratchVertices;
};

}  // namespace Spark
