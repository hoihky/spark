#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/RootMotionDelta.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class Skeleton;

/**
 * Samples motion-joint positions from playback state and computes per-frame deltas.
 * Single responsibility: animation-space root motion math (no transform writes).
 */
class RootMotionSampler {
public:
    [[nodiscard]] bool TrySampleMotionJointPosition(
            const AnimatorComponent& animator,
            std::uint32_t jointIndex,
            Vector3& outPosition) const;

    /**
     * Computes translation delta between the previous and current playback times.
     * Handles loop wrap for looping clips; returns <c>valid == false</c> on the first sample.
     */
    [[nodiscard]] bool TryComputeDelta(
            const AnimatorComponent& animator,
            std::uint32_t jointIndex,
            float deltaSeconds,
            Vector3& inOutPreviousPosition,
            bool& inOutHasPreviousSample,
            RootMotionDelta& outDelta) const;

private:
    [[nodiscard]] bool TrySampleJointPositionAtTime(
            const Skeleton& skeleton,
            std::uint32_t jointIndex,
            std::uint32_t clipIndex,
            float timeSeconds,
            AnimLoopMode loopMode,
            Vector3& outPosition) const;

    [[nodiscard]] bool TrySampleBlendedPosition(
            const Skeleton& skeleton,
            std::uint32_t jointIndex,
            std::uint32_t clipA,
            float timeA,
            std::uint32_t clipB,
            float timeB,
            float blendB,
            Vector3& outPosition) const;
};

}  // namespace Spark
