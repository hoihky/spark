#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Single threshold → clip mapping for a 1D blend tree. Keyframes must be sorted by ascending threshold. */
struct AnimBlend1DKeyframe {
    float threshold = 0.0F;
    std::uint32_t clipIndex = 0;
};

/** Result of sampling a 1D blend tree at a scalar parameter (e.g. locomotion speed m/s). */
struct AnimBlend1DSample {
    std::uint32_t clipA = 0;
    std::uint32_t clipB = 0;
    /** 0 = full clipA, 1 = full clipB. When clipA == clipB, blend is ignored. */
    float blend01 = 0.0F;
    bool valid = false;
};

/**
 * Evaluates a sorted 1D blend tree. Clamps below the first and above the last keyframe.
 * Returns <c>valid == false</c> when <c>keyframeCount == 0</c>.
 */
[[nodiscard]] AnimBlend1DSample EvaluateAnimBlend1D(
        const AnimBlend1DKeyframe* keyframes,
        std::size_t keyframeCount,
        float parameter) noexcept;

}  // namespace Spark
