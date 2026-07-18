#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

/**
 * CPU cluster builder + host-mapped SSBOs for clustered forward punctual lighting.
 * Descriptor bindings: 4 = lights, 5 = cluster offsets/counts/indices.
 */
class VulkanClusteredForwardLights {
public:
    void CreateBuffers(VkPhysicalDevice physicalDevice, VkDevice device, std::uint32_t framesInFlight);
    void DestroyBuffers(VkDevice device);

    void BuildAndUpload(
            std::uint32_t frameIndex,
            const SceneRenderParams& scene,
            const ResolvedSceneLighting& lighting,
            VkExtent2D extent);

    [[nodiscard]] VkBuffer LightsBuffer(std::uint32_t frameIndex) const noexcept;
    [[nodiscard]] VkBuffer ClusterBuffer(std::uint32_t frameIndex) const noexcept;

private:
    VkDevice device = VK_NULL_HANDLE;
    std::uint32_t framesInFlight = 0;

    struct Flight {
        VkBuffer lightsBuffer = VK_NULL_HANDLE;
        VkDeviceMemory lightsMemory = VK_NULL_HANDLE;
        void* lightsMapped = nullptr;

        VkBuffer clusterBuffer = VK_NULL_HANDLE;
        VkDeviceMemory clusterMemory = VK_NULL_HANDLE;
        void* clusterMapped = nullptr;
    };

    Flight flights[2]{};
};

}  // namespace Spark
