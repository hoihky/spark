#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Spark {

class VulkanClusteredForwardLights;
class VulkanDirectionalShadowPass;
class VulkanPunctualShadowPass;
class VulkanSceneTextureUploader;
class VulkanSpritePass;

/**
 * Scene descriptor set layout, per-flight uniform/skin SSBOs, pool, and bound descriptor sets.
 * Binding slots 0–9 match the lit-scene shader layout (UBO, textures, lights, shadows, sprites).
 */
class VulkanSceneDescriptors {
public:
    static constexpr std::uint32_t kMaxSkinJoints = 64;
    /** std430 mat4 skinPalette[kMaxSkinJoints] — 64 joints × 64 bytes. */
    static constexpr VkDeviceSize kSkinSsboBytes = 4096;

    struct BindingSources {
        const VulkanSceneTextureUploader& sceneTextureUploader;
        const VulkanClusteredForwardLights& clusteredForwardLights;
        const VulkanDirectionalShadowPass& directionalShadow;
        const VulkanPunctualShadowPass& punctualShadow;
        const VulkanSpritePass& spritePass;
    };

    void CreateSetLayout(VkDevice device);
    void CreateUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void CreateSkinSsboBuffers(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void CreatePoolAndSets(VkDevice device, std::uint32_t framesInFlight, const BindingSources& sources);
    void Destroy(VkDevice device) noexcept;

    [[nodiscard]] VkDescriptorSetLayout Layout() const noexcept { return descriptorSetLayout; }
    [[nodiscard]] std::size_t DescriptorSetCount() const noexcept { return descriptorSets.GetSize(); }
    [[nodiscard]] VkDescriptorSet DescriptorSet(std::uint32_t frameIndex) const noexcept {
        return frameIndex < descriptorSets.GetSize() ? descriptorSets[frameIndex] : VK_NULL_HANDLE;
    }
    [[nodiscard]] VkBuffer UniformBuffer(std::uint32_t frameIndex) const noexcept {
        return frameIndex < uniformBuffers.GetSize() ? uniformBuffers[frameIndex] : VK_NULL_HANDLE;
    }
    [[nodiscard]] void* UniformMapped(std::uint32_t frameIndex) const noexcept {
        return frameIndex < uniformBuffersMapped.GetSize() ? uniformBuffersMapped[frameIndex] : nullptr;
    }
    [[nodiscard]] Array<void*>& SkinSsboMapped() noexcept { return skinSsboMapped; }
    [[nodiscard]] const Array<void*>& SkinSsboMapped() const noexcept { return skinSsboMapped; }

private:
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> descriptorSets;

    Array<VkBuffer> uniformBuffers;
    Array<VkDeviceMemory> uniformBuffersMemory;
    Array<void*> uniformBuffersMapped;

    Array<VkBuffer> skinSsboBuffers;
    Array<VkDeviceMemory> skinSsboMemory;
    Array<void*> skinSsboMapped;
};

}  // namespace Spark
