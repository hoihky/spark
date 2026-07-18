#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

/**
 * Pure directional CSM math (no Vulkan). Used by scene UBO packing and shadow pass setup.
 * See comment block in <c>VulkanDirectionalShadowCascadeMath.cpp</c> for pipeline trace.
 */
class VulkanDirectionalShadowCascadeMath {
public:
    [[nodiscard]] static const Matrix4& ShadowClipLinearZFlip() noexcept;

    [[nodiscard]] static float ComputeSplitDistance(
            float nearDist,
            float farDist,
            int splitIndex,
            int splitCount,
            float lambda) noexcept;

    [[nodiscard]] static bool ComputeWorldToShadowClip(
            const Matrix4& viewProj,
            const Vector3& lightDirectionWorldTowardLight,
            float distNear,
            float distFar,
            std::uint32_t cascadeTileSize,
            Matrix4& outWorldToShadowClip);
};

}  // namespace Spark
