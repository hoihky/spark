#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"
#include "spark/render/scene/VulkanSceneMeshGpu.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class VulkanCustomMeshPool;

struct VulkanFoliageRecordContext {
    const SceneRenderParams* scene = nullptr;
    bool sceneParamsValid = false;
    std::uint32_t frameIndex = 0;
    VkExtent2D extent{};
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    const VulkanCustomMeshPool* customMeshPool = nullptr;
    VkBuffer customVertexBuffer = VK_NULL_HANDLE;
    VkBuffer customIndexBuffer = VK_NULL_HANDLE;
};

/** Instanced alpha-tested foliage meshes (F1), after opaque lit pass. */
class VulkanFoliageInstancedPass {
public:
    void CreateGpuResources(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void DestroyGpuResources(VkDevice device);

    void CreateGraphicsPipeline(
            VkDevice device,
            VkRenderPass hdrRenderPass,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    void Record(VkCommandBuffer commandBuffer, const VulkanFoliageRecordContext& ctx) const;

    [[nodiscard]] VkBuffer InstanceBuffer(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const noexcept { return pipelineLayout; }

private:
    struct FoliageBatchPushConstants {
        std::uint32_t instanceBase = 0;
        std::int32_t textureLayer = -1;
        float alphaCutoff = 0.35F;
        float bladeHeight = 0.72F;
        float windBendScale = 0.55F;
        float padding0 = 0.0F;
        float padding1 = 0.0F;
        float albedoTint[4]{1.0F, 1.0F, 1.0F, 1.0F};
    };

    [[nodiscard]] bool UploadInstances(std::uint32_t frameIndex, const SceneRenderParams& scene) const;

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    Array<VkBuffer> instanceBuffers;
    Array<VkDeviceMemory> instanceMemory;
    Array<void*> instanceMapped;
};

}  // namespace Spark
