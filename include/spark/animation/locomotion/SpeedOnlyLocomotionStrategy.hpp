#pragma once

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/ILocomotionPlaybackStrategy.hpp"

#include <cstdint>

namespace Spark {

/** Legacy idle / walk / run selection from planar speed (1D blend tree). */
class SpeedOnlyLocomotionStrategy final : public ILocomotionPlaybackStrategy {
public:
    void SetIdleClip(const std::uint32_t clip) noexcept { idleClip = clip; }
    void SetWalkClip(const std::uint32_t clip) noexcept { walkClip = clip; }
    void SetRunClip(const std::uint32_t clip) noexcept { runClip = clip; }

    void BuildPlayback(
            const LocomotionPlanarSample& sample,
            const AnimatorComponent& anim,
            float walkSpeedThreshold,
            float runSpeedThreshold,
            bool wantSprint,
            LocomotionPlaybackDecision& outDecision) const noexcept override;

private:
    std::uint32_t idleClip = 0;
    std::uint32_t walkClip = 1;
    std::uint32_t runClip = kInvalidAnimClipIndex;
};

}  // namespace Spark
