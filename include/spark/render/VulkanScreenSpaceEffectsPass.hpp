#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/VulkanDepthResources.hpp"
#include "spark/render/VulkanHdrTonemapPass.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** SSAO composite pass (HDR → scratch, before tonemap). */
class VulkanScreenSpaceEffectsPass {
public:
    static constexpr VkFormat kColorFormat = VulkanHdrTonemapPass::kColorFormat;

    struct FlightTarget {
        VkImage colorImage = VK_NULL_HANDLE;
        VkDeviceMemory colorMemory = VK_NULL_HANDLE;
        VkImageView colorView = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkImageLayout colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        /** Depth copied from the scene pass — safe to sample (main depth stays attachment-only). */
        VkImage depthSampleImage = VK_NULL_HANDLE;
        VkDeviceMemory depthSampleMemory = VK_NULL_HANDLE;
        VkImageView depthSampleView = VK_NULL_HANDLE;
        VkImageLayout depthSampleLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void CreateRenderPass(VkDevice device);
    void DestroyRenderPass(VkDevice device);

    void RecreateFlightTargets(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkExtent2D extent,
            VkFormat depthFormat,
            std::uint32_t framesInFlight);
    void DestroyFlightTargets(VkDevice device);

    void CreatePipeline(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            std::uint32_t framesInFlight,
            const VulkanSpvShaderLoader& shaders);
    void DestroyPipeline(VkDevice device);
    void UpdateDescriptor(
            VkDevice device,
            std::uint32_t flightIndex,
            VkBuffer sceneUbo,
            const VulkanHdrTonemapPass::FlightTarget& hdrFlight,
            const FlightTarget& effectsFlight);

    void Record(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const VulkanDepthResources& sceneDepth,
            const SceneRenderParams& scene);

    [[nodiscard]] bool HasFlight(std::uint32_t frameIndex) const noexcept {
        return frameIndex < flights.GetSize();
    }
    [[nodiscard]] VkImageView OutputView(std::uint32_t frameIndex) const noexcept {
        return (frameIndex < flights.GetSize()) ? flights[frameIndex].colorView : VK_NULL_HANDLE;
    }
    [[nodiscard]] const FlightTarget& Flight(std::uint32_t frameIndex) const noexcept {
        return flights[frameIndex];
    }

private:
    struct PostPushConstants {
        float ssaoEnabled = 0.0F;
        float ssaoRadius = 0.35F;
        float ssaoBias = 0.02F;
        float ssaoStrength = 0.65F;
        float depthFlipV = 0.0F;
        float pad0 = 0.0F;
        float pad1 = 0.0F;
        float pad2 = 0.0F;
    };

    static PostPushConstants BuildPushConstants(const SceneRenderParams& scene);

    void RecordDepthCopy(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const VulkanDepthResources& sceneDepth);

    VkRenderPass renderPass = VK_NULL_HANDLE;
    Array<FlightTarget> flights;
    VkSampler colorSampler = VK_NULL_HANDLE;
    VkSampler depthSampler = VK_NULL_HANDLE;

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> descriptorSets;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
};

}  // namespace Spark
