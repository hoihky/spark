#pragma once

#include <cstdint>

namespace Spark {

/** Interleaved scene mesh vertex: position, normal, uv, tangent, joints, weights. */
struct VulkanSceneVertexLayout {
    static constexpr std::uint32_t kFloatsPerVertex = 20U;
    static constexpr std::uint32_t kStrideBytes = kFloatsPerVertex * sizeof(float);
    static constexpr std::uint32_t kOffPosition = 0U;
    static constexpr std::uint32_t kOffNormal = 3U;
    static constexpr std::uint32_t kOffTexCoord = 6U;
    static constexpr std::uint32_t kOffTangent = 8U;
    static constexpr std::uint32_t kOffJoints = 12U;
    static constexpr std::uint32_t kOffWeights = 16U;
};

}  // namespace Spark
