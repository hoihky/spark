#pragma once

#include "spark/animation/AnimLoopMode.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class LocomotionPlaybackDecision;

/** Applies a <c>LocomotionPlaybackDecision</c> to an <c>AnimatorComponent</c>. */
class LocomotionPlaybackApplicator final {
public:
    void SetCrossfadeDuration(const float seconds) noexcept { crossfadeDuration = seconds; }

    void Apply(AnimatorComponent& anim, const LocomotionPlaybackDecision& decision) const noexcept;

private:
    void ApplySingleClip_(AnimatorComponent& anim, std::uint32_t clip, AnimLoopMode loopMode) const noexcept;

    float crossfadeDuration = 0.18F;
};

}  // namespace Spark
