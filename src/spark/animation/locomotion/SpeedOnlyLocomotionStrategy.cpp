#include "spark/animation/locomotion/SpeedOnlyLocomotionStrategy.hpp"

#include "spark/animation/AnimBlend1D.hpp"
#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"

namespace Spark {

void SpeedOnlyLocomotionStrategy::BuildPlayback(
        const LocomotionPlanarSample& sample,
        const AnimatorComponent& anim,
        const float walkSpeedThreshold,
        const float runSpeedThreshold,
        const bool wantSprint,
        LocomotionPlaybackDecision& outDecision) const noexcept {
    const auto valid = [&](const std::uint32_t clip) { return clip < anim.GetClipCount(); };

    float speedParam = sample.GetPlanarSpeed();
    if (speedParam <= 1.0e-4F && wantSprint) {
        speedParam = walkSpeedThreshold;
    }

    AnimBlend1DKeyframe keyframes[3]{};
    std::size_t keyframeCount = 0;
    if (valid(idleClip)) {
        keyframes[keyframeCount++] = {0.0F, idleClip};
    }
    if (valid(walkClip)) {
        keyframes[keyframeCount++] = {walkSpeedThreshold, walkClip};
    }
    if (valid(runClip) && wantSprint) {
        keyframes[keyframeCount++] = {runSpeedThreshold, runClip};
    }

    if (keyframeCount == 0) {
        outDecision.SetIdle();
        return;
    }

    const AnimBlend1DSample blendSample = EvaluateAnimBlend1D(keyframes, keyframeCount, speedParam);
    if (!blendSample.valid) {
        outDecision.SetIdle();
        return;
    }

    if (blendSample.clipA == blendSample.clipB) {
        outDecision.SetSingleClip(blendSample.clipA, AnimLoopMode::Loop);
        return;
    }
    outDecision.SetSpeedBlend(blendSample.clipA, blendSample.clipB, blendSample.blend01);
}

}  // namespace Spark
