#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class Window;

/** Vulkan objects required to (re)create the Dear ImGui device backend. */
struct ImGuiVulkanBackendInfo {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    std::uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::uint32_t minImageCount = 2;
    std::uint32_t imageCount = 2;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

/**
 * GPU recording hook used by <c>VulkanRenderer</c> (keeps ImGui types out of the renderer header).
 * Implemented inside <c>ImGuiVulkanLayer</c>.
 */
class IImGuiVulkanBackend {
public:
    virtual ~IImGuiVulkanBackend() = default;

    virtual void InitGlfw(Window& window) = 0;
    virtual void InitVulkan(const ImGuiVulkanBackendInfo& info) = 0;
    virtual void InvalidateVulkan() noexcept = 0;
    virtual void RecreateVulkan(const ImGuiVulkanBackendInfo& info) = 0;
    virtual void Shutdown() noexcept = 0;
    virtual void RecordDrawData(VkCommandBuffer commandBuffer) = 0;
};

}  // namespace Spark
