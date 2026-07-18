#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowFrameState.hpp"
#include "spark/render/scene/VulkanSceneMeshGpu.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** Inputs required to record shadow-map geometry (owned elsewhere; non-owning refs). */
struct VulkanShadowRecordContext {
    const SceneRenderParams* scene = nullptr;
    bool sceneParamsValid = false;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkBuffer customVertexBuffer = VK_NULL_HANDLE;
    VkBuffer customIndexBuffer = VK_NULL_HANDLE;
    std::uint32_t cubeIndexCount = 0;
    std::uint32_t planeIndexCount = 0;
    std::uint32_t planeFirstIndex = 0;
    std::int32_t cubeVertexOffset = 0;
    std::int32_t planeVertexOffset = 0;
    const Array<CustomMeshGpuSlice>* customDrawPacked = nullptr;
    const Array<void*>* skinSsboMapped = nullptr;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    std::uint32_t maxSkinJoints = 64;
};

/**
 * Directional CSM shadow atlas: resources, pipeline, and per-frame depth recording.
 * Composed by <c>VulkanRenderer</c>; does not own scene geometry or descriptor pools.
 */
class VulkanDirectionalShadowPass {
public:
    static constexpr std::uint32_t kMapSize = 2048;
    static constexpr std::uint32_t kCascadeCount = 4;
    static constexpr std::uint32_t kCascadeTileSize = 1024;

    struct FlightTarget {
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void CreateResources(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void DestroyResources(VkDevice device);

    void CreateGraphicsPipeline(
            VkDevice device,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    void Record(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            const VulkanShadowRecordContext& ctx,
            const VulkanDirectionalShadowFrameState& frameState);

    [[nodiscard]] bool HasFlightDepthView(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkSampler CompareSampler() const noexcept { return compareSampler; }
    [[nodiscard]] const FlightTarget& Flight(std::uint32_t frameIndex) const noexcept { return flights[frameIndex]; }

private:
    void EnsureDepthImageReadable(VkCommandBuffer commandBuffer, FlightTarget& flight) const;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    Array<FlightTarget> flights;
    VkSampler compareSampler = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkShaderModule vertModule = VK_NULL_HANDLE;
};

}  // namespace Spark
