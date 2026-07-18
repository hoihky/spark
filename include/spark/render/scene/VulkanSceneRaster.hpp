#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Front face for CCW-authored meshes after <c>PerspectiveVulkan</c> / <c>OrthographicVulkan</c> clip-Y flip. */
inline VkFrontFace VulkanSceneFrontFaceForModel(const Matrix4& model) noexcept {
    const float det = model.DeterminantUpper3x3();
    // View-space CCW becomes clockwise in the framebuffer once projection negates clip Y.
    return (det < 0.0F) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
}

/** Sets dynamic cull/front-face state required by <c>VulkanScenePipeline</c> before each indexed draw. */
inline void VulkanSceneApplyRasterState(
        VkCommandBuffer commandBuffer,
        const SceneDrawItem& item,
        SceneSkyMode skyMode) noexcept {
    if (skyMode != SceneSkyMode::None) {
        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
        vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
        return;
    }
    if (item.doubleSided || item.mesh == SceneMeshSlot::GroundPlane) {
        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
    } else {
        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
    }
    vkCmdSetFrontFace(commandBuffer, VulkanSceneFrontFaceForModel(item.model));
}

}  // namespace Spark
