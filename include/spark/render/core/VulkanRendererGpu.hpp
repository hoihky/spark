#pragma once

#include "spark/render/gpu/VulkanGpuBufferImage.hpp"
#include "spark/render/gpu/VulkanGpuInstance.hpp"
#include "spark/render/gpu/VulkanGpuMeshInterleaved.hpp"
#include "spark/render/gpu/VulkanGpuPhysicalDevice.hpp"
#include "spark/render/gpu/VulkanGpuSwapchain.hpp"

#include "spark/core/Array.hpp"

#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Spark {

class Window;

namespace VulkanRendererGpu {

/** Mirrors <c>VulkanGpuInstance::kEnableValidationLayers</c> for call sites that already use this name. */
constexpr bool kEnableValidationLayers = VulkanGpuInstance::kEnableValidationLayers;

/** Mirrors <c>VulkanGpuInstance::kKhronosValidationLayerName</c>. */
constexpr const char* kKhronosValidationLayerName = VulkanGpuInstance::kKhronosValidationLayerName;

/** PFN for validation debug output (same as <c>VulkanGpuDefaultDebugMessengerCallback</c>). */
inline constexpr PFN_vkDebugUtilsMessengerCallbackEXT DefaultDebugMessengerCallback =
        VulkanGpuDefaultDebugMessengerCallback;

[[nodiscard]] inline bool CheckValidationLayerSupport() {
    return VulkanGpuInstance::CheckValidationLayerSupport();
}

[[nodiscard]] inline Array<const char*> GetRequiredInstanceExtensions(const Window& appWindow) {
    return VulkanGpuInstance::GetRequiredInstanceExtensions(appWindow);
}

[[nodiscard]] inline VkResult CreateDebugUtilsMessenger(
        VkInstance vkInstance,
        const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* outMessenger) {
    return VulkanGpuInstance::CreateDebugUtilsMessenger(vkInstance, createInfo, allocator, outMessenger);
}

inline void DestroyDebugUtilsMessenger(
        VkInstance vkInstance,
        VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* allocator) {
    VulkanGpuInstance::DestroyDebugUtilsMessenger(vkInstance, messenger, allocator);
}

[[nodiscard]] inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    return VulkanGpuPhysicalDevice::FindQueueFamilies(physicalDevice, surface);
}

[[nodiscard]] inline bool DeviceExtensionSupported(VkPhysicalDevice physicalDevice, const char* extensionName) {
    return VulkanGpuPhysicalDevice::DeviceExtensionSupported(physicalDevice, extensionName);
}

[[nodiscard]] inline bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    return VulkanGpuPhysicalDevice::IsDeviceSuitable(physicalDevice, surface);
}

[[nodiscard]] inline SwapchainSupportDetails QuerySwapchainSupport(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface) {
    return VulkanGpuSwapchain::QuerySwapchainSupport(physicalDevice, surface);
}

[[nodiscard]] inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const Array<VkSurfaceFormatKHR>& formats) {
    return VulkanGpuSwapchain::ChooseSwapSurfaceFormat(formats);
}

[[nodiscard]] inline VkPresentModeKHR ChooseSwapPresentMode(const Array<VkPresentModeKHR>& presentModes) {
    return VulkanGpuSwapchain::ChooseSwapPresentMode(presentModes);
}

[[nodiscard]] inline VkExtent2D ChooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        GLFWwindow* glfwWindow) {
    return VulkanGpuSwapchain::ChooseSwapExtent(capabilities, glfwWindow);
}

[[nodiscard]] inline std::uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        std::uint32_t typeFilter,
        VkMemoryPropertyFlags properties) {
    return VulkanGpuBufferImage::FindMemoryType(physicalDevice, typeFilter, properties);
}

inline void CreateBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice vkDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory) {
    VulkanGpuBufferImage::CreateBuffer(physicalDevice, vkDevice, size, usage, properties, buffer, bufferMemory);
}

inline void CopyBuffer(
        VkDevice vkDevice,
        VkCommandPool commandPool,
        VkQueue queue,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size) {
    VulkanGpuBufferImage::CopyBuffer(vkDevice, commandPool, queue, srcBuffer, dstBuffer, size);
}

[[nodiscard]] inline VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
    return VulkanGpuBufferImage::FindDepthFormat(physicalDevice);
}

inline void CreateImage(
        VkPhysicalDevice physicalDevice,
        VkDevice vkDevice,
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory) {
    VulkanGpuBufferImage::CreateImage(
            physicalDevice, vkDevice, width, height, format, tiling, usage, properties, image, imageMemory);
}

inline void CreateImage2DArray(
        VkPhysicalDevice physicalDevice,
        VkDevice vkDevice,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t arrayLayers,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory,
        std::uint32_t mipLevels = 1) {
    VulkanGpuBufferImage::CreateImage2DArray(
            physicalDevice,
            vkDevice,
            width,
            height,
            arrayLayers,
            format,
            tiling,
            usage,
            properties,
            image,
            imageMemory,
            mipLevels);
}

[[nodiscard]] inline VkImageAspectFlags ImageAspectForFormat(const VkFormat format) noexcept {
    return VulkanGpuBufferImage::ImageAspectForFormat(format);
}

[[nodiscard]] inline VkImageView CreateImageView2DArray(
        VkDevice vkDevice,
        VkImage image,
        VkFormat format,
        const std::uint32_t layerCount,
        const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        const std::uint32_t mipLevelCount = 1) {
    return VulkanGpuBufferImage::CreateImageView2DArray(
            vkDevice, image, format, layerCount, aspectMask, mipLevelCount);
}

[[nodiscard]] inline VkSampler CreateTextureSampler(VkDevice vkDevice, const float maxLod = 0.0F) {
    return VulkanGpuBufferImage::CreateTextureSampler(vkDevice, maxLod);
}

[[nodiscard]] inline VkSampler CreateSpriteSceneTextureSampler(VkDevice vkDevice) {
    return VulkanGpuBufferImage::CreateSpriteSceneTextureSampler(vkDevice);
}

[[nodiscard]] inline VkSampler CreateFontAtlasSampler(VkDevice vkDevice) {
    return VulkanGpuBufferImage::CreateFontAtlasSampler(vkDevice);
}

inline void SceneTexBarrier(
        VkCommandBuffer cmd,
        VkImage image,
        std::uint32_t layerCount,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        const std::uint32_t mipLevelCount = 1) {
    VulkanGpuBufferImage::SceneTexBarrier(cmd, image, layerCount, oldLayout, newLayout, mipLevelCount);
}

inline void GenerateMipmapsBlit(
        VkCommandBuffer cmd,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t baseArrayLayer,
        std::uint32_t layerCount,
        std::uint32_t mipLevelCount) {
    VulkanGpuBufferImage::GenerateMipmapsBlit(
            cmd, image, width, height, baseArrayLayer, layerCount, mipLevelCount);
}

inline void GenerateMipmapsNearestFromBase(
        VkCommandBuffer cmd,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t baseArrayLayer,
        std::uint32_t layerCount,
        std::uint32_t mipLevelCount) {
    VulkanGpuBufferImage::GenerateMipmapsNearestFromBase(
            cmd, image, width, height, baseArrayLayer, layerCount, mipLevelCount);
}

inline void AppendRigidMeshVertexToInterleaved(const Mesh::Vertex& v, Array<float>& interleaved) {
    VulkanGpuMeshInterleaved::AppendRigidMeshVertexToInterleaved(v, interleaved);
}

inline void AppendSkinnedVertexToInterleaved(const SkinnedMesh::Vertex& v, Array<float>& interleaved) {
    VulkanGpuMeshInterleaved::AppendSkinnedVertexToInterleaved(v, interleaved);
}

template<typename Fn>
void RunOneTimeCommands(VkDevice vkDevice, VkCommandPool commandPool, VkQueue queue, Fn&& fn) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("RunOneTimeCommands: allocate failed");
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    fn(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("RunOneTimeCommands: submit failed");
    }
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
}

}  // namespace VulkanRendererGpu

}  // namespace Spark
