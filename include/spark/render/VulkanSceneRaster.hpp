#pragma once

#include "spark/math/Matrix4.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Front face for CCW-authored meshes after <c>PerspectiveVulkan</c> / <c>OrthographicVulkan</c> clip-Y flip. */
inline VkFrontFace VulkanSceneFrontFaceForModel(const Matrix4& model) noexcept {
    const float det = model.DeterminantUpper3x3();
    // View-space CCW becomes clockwise in the framebuffer once projection negates clip Y.
    return (det < 0.0F) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
}

}  // namespace Spark
