#pragma once

#include <cstdint>

namespace Spark {

/** Interleaved scene mesh vertex: position, normal, uv0, uv1, tangent, color, joints, weights. */
struct VulkanSceneVertexLayout {
    static constexpr std::uint32_t kFloatsPerVertex = 26U;
    static constexpr std::uint32_t kStrideBytes = kFloatsPerVertex * sizeof(float);
    static constexpr std::uint32_t kOffPosition = 0U;
    static constexpr std::uint32_t kOffNormal = 3U;
    static constexpr std::uint32_t kOffTexCoord0 = 6U;
    static constexpr std::uint32_t kOffTexCoord1 = 8U;
    static constexpr std::uint32_t kOffTangent = 10U;
    static constexpr std::uint32_t kOffColor = 14U;
    static constexpr std::uint32_t kOffJoints = 18U;
    static constexpr std::uint32_t kOffWeights = 22U;
};

}  // namespace Spark
