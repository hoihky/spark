#pragma once

#include "spark/core/Array.hpp"
#include "spark/render/present/VulkanPresentationFramebuffers.hpp"
#include "spark/render/present/VulkanPresentRenderPass.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/** HDR offscreen targets (R16G16B16A16) and swapchain tonemap pass. */
class VulkanHdrTonemapPass {
public:
    static constexpr VkFormat kColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    struct FlightTarget {
        VkImage colorImage = VK_NULL_HANDLE;
        VkDeviceMemory colorMemory = VK_NULL_HANDLE;
        VkImageView colorView = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkImageLayout colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    void CreateRenderPass(VkDevice device, VkFormat depthFormat);
    void DestroyRenderPass(VkDevice device);

    void RecreateFlightTargets(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkExtent2D extent,
            std::uint32_t framesInFlight,
            const VkImageView* depthViews,
            std::size_t depthViewCount);
    void DestroyFlightTargets(VkDevice device);

    void CreateTonemapPipeline(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkRenderPass presentRenderPass,
            std::uint32_t framesInFlight,
            const VulkanSpvShaderLoader& shaders);
    void DestroyTonemapPipeline(VkDevice device);
    void UpdateTonemapDescriptor(
            VkDevice device,
            std::uint32_t flightIndex,
            VkImageView colorViewOverride = VK_NULL_HANDLE);

    void BeginColorAttachmentBarrierIfNeeded(VkCommandBuffer commandBuffer, std::uint32_t frameIndex);
    void TransitionColorToShaderRead(VkCommandBuffer commandBuffer, std::uint32_t frameIndex);

    void RecordTonemap(
            VkCommandBuffer commandBuffer,
            std::uint32_t imageIndex,
            std::uint32_t flightIndex,
            VkRenderPass presentRenderPass,
            VkExtent2D swapchainExtent,
            const VulkanPresentationFramebuffers& presentationFramebuffers,
            float exposure);

    [[nodiscard]] VkRenderPass HdrRenderPass() const noexcept { return hdrRenderPass; }
    [[nodiscard]] FlightTarget& Flight(std::uint32_t frameIndex) noexcept { return flights[frameIndex]; }
    [[nodiscard]] const FlightTarget& Flight(std::uint32_t frameIndex) const noexcept { return flights[frameIndex]; }
    [[nodiscard]] bool HasFlight(std::uint32_t frameIndex) const noexcept { return frameIndex < flights.GetSize(); }

private:
    struct TonemapPushConstants {
        float exposure = 0.68F;
        float invGamma = 1.0F / 2.35F;
        float pad0 = 0.0F;
        float pad1 = 0.0F;
    };

    VkRenderPass hdrRenderPass = VK_NULL_HANDLE;
    Array<FlightTarget> flights;
    VkSampler colorSampler = VK_NULL_HANDLE;

    VkShaderModule tonemapVertModule = VK_NULL_HANDLE;
    VkShaderModule tonemapFragModule = VK_NULL_HANDLE;
    VkPipeline tonemapPipeline = VK_NULL_HANDLE;
    VkPipelineLayout tonemapPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout tonemapDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool tonemapDescriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> tonemapDescriptorSets;
    VkBuffer tonemapVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory tonemapVertexMemory = VK_NULL_HANDLE;
};

}  // namespace Spark
