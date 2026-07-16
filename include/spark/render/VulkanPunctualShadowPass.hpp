#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/SceneLightingProfile.hpp"
#include "spark/render/VulkanDirectionalShadowPass.hpp"
#include "spark/render/VulkanPunctualShadowFrameState.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * Spot shadow atlas (4×512² tiles in 1024²) + point shadow depth array (2 lights × 6 faces).
 * Descriptor bindings: 6 = metadata SSBO, 7 = spot atlas, 8 = point depth array.
 */
class VulkanPunctualShadowPass {
public:
    struct SpotFlightTarget {
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct PointFlightTarget {
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthArrayView = VK_NULL_HANDLE;
        Array<VkImageView> layerViews;
        Array<VkFramebuffer> layerFramebuffers;
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct FlightTarget {
        SpotFlightTarget spot{};
        PointFlightTarget point{};
        VkBuffer ssboBuffer = VK_NULL_HANDLE;
        VkDeviceMemory ssboMemory = VK_NULL_HANDLE;
        void* ssboMapped = nullptr;
    };

    void CreateResources(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void DestroyResources(VkDevice device);

    void CreateGraphicsPipeline(
            VkDevice device,
            VkDescriptorSetLayout sceneDescriptorSetLayout,
            const VulkanSpvShaderLoader& shaders);
    void DestroyGraphicsPipeline(VkDevice device);

    void PrepareAndUpload(
            std::uint32_t frameIndex,
            const SceneRenderParams& scene,
            const ResolvedSceneLighting& lighting,
            VulkanPunctualShadowFrameState& out);

    void Record(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            const VulkanShadowRecordContext& ctx,
            const VulkanPunctualShadowFrameState& frameState);

    [[nodiscard]] bool HasFlightResources(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkSampler CompareSampler() const noexcept { return compareSampler; }
    [[nodiscard]] const FlightTarget& Flight(std::uint32_t frameIndex) const noexcept { return flights[frameIndex]; }
    [[nodiscard]] VkBuffer SsboBuffer(std::uint32_t frameIndex) const noexcept;

private:
    void EnsureDepthImagesReadable(VkCommandBuffer commandBuffer, FlightTarget& flight) const;
    void TransitionDepthImage(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageLayout& layoutInOut,
            std::uint32_t layerCount,
            VkImageLayout newLayout,
            VkAccessFlags dstAccess) const;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    Array<FlightTarget> flights;
    VkSampler compareSampler = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkShaderModule vertModule = VK_NULL_HANDLE;
};

}  // namespace Spark
