#pragma once

namespace Spark {

class AnimatorComponent;
class LocomotionPlaybackDecision;
class LocomotionPlanarSample;

class ILocomotionPlaybackStrategy {
public:
    virtual ~ILocomotionPlaybackStrategy() = default;

    virtual void BuildPlayback(
            const LocomotionPlanarSample& sample,
            const AnimatorComponent& anim,
            float walkSpeedThreshold,
            float runSpeedThreshold,
            bool wantSprint,
            LocomotionPlaybackDecision& outDecision) const noexcept = 0;
};

}  // namespace Spark
