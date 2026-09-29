#pragma once

#include "spark/render/lighting/SceneLightingResolver.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowFrameState.hpp"
#include "spark/render/scene/VulkanSceneDescriptors.hpp"
#include "spark/render/scene/VulkanSceneUniformWriter.hpp"
#include "spark/render/scene/VulkanRenderTargetRegistry.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowPass.hpp"
#include "spark/render/sprites2d/Vulkan2DCompositePass.hpp"
#include "spark/render/sprites2d/VulkanSpritePass.hpp"
#include "spark/render/sprites2d/VulkanTilemapPass.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class SceneRenderParams;

/** Records orthographic 2D composite resubmits into <c>RenderTexture</c> targets (minimap, etc.). */
class VulkanScene2DCompositeCapture final {
public:
    struct RecordContext {
        const SceneRenderParams* scene = nullptr;
        bool sceneParamsValid = false;
        std::uint32_t frameIndex = 0;
        VkRenderPass hdrRenderPass = VK_NULL_HANDLE;
        VulkanRenderTargetRegistry* renderTargets = nullptr;
        VulkanSceneDescriptors* descriptors = nullptr;
        VulkanSceneUniformWriter* uniformWriter = nullptr;
        const ResolvedSceneLighting* lighting = nullptr;
        VulkanDirectionalShadowPass* directionalShadow = nullptr;
        VulkanDirectionalShadowFrameState* shadowFrameState = nullptr;
        VulkanTilemapPass* tilemapPass = nullptr;
        VulkanSpritePass* spritePass = nullptr;
        Vulkan2DCompositePass* compositePass = nullptr;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        std::uint32_t quadFirstIndex = 0;
        std::uint32_t quadIndexCount = 0;
        VkExtent2D presentExtent{};
    };

    void Record(VkCommandBuffer commandBuffer, const RecordContext& ctx) const noexcept;
};

}  // namespace Spark
