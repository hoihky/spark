#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowFrameState.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowPass.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Packs scene uniform buffer (std140) and directional shadow cascade state for the current frame. */
class VulkanSceneUniformWriter {
public:
    void Write(
            void* mappedUbo,
            const SceneRenderParams& scene,
            const ResolvedSceneLighting& lighting,
            VkExtent2D swapchainExtent,
            const VulkanDirectionalShadowPass& shadowPass,
            std::uint32_t frameIndex,
            VulkanDirectionalShadowFrameState& shadowOut) const;
};

}  // namespace Spark
