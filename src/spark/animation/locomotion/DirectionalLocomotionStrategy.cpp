#include "spark/animation/locomotion/DirectionalLocomotionStrategy.hpp"

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"

namespace Spark {

void DirectionalLocomotionStrategy::BuildPlayback(
        const LocomotionPlanarSample& sample,
        const AnimatorComponent& anim,
        const float walkSpeedThreshold,
        const float runSpeedThreshold,
        const bool wantSprint,
        LocomotionPlaybackDecision& outDecision) const noexcept {
    if (!catalog.HasDirectionalLocomotion(anim)) {
        outDecision.SetIdle();
        return;
    }

    const float speed = sample.GetPlanarSpeed();
    if (speed <= 1.0e-3F) {
        if (catalog.GetIdleClip() < anim.GetClipCount()) {
            outDecision.SetSingleClip(catalog.GetIdleClip(), AnimLoopMode::Loop);
        } else {
            outDecision.SetIdle();
        }
        return;
    }

    std::uint32_t clipA = 0;
    std::uint32_t clipB = 0;
    float blend = 0.0F;
    cardinalMixer.Evaluate(
            sample.GetSignedVelocityYawRelativeToBody(),
            speed,
            walkSpeedThreshold,
            runSpeedThreshold,
            wantSprint,
            clipA,
            clipB,
            blend);

    if (clipA >= anim.GetClipCount() || clipB >= anim.GetClipCount()) {
        outDecision.SetIdle();
        return;
    }

    if (clipA == clipB) {
        outDecision.SetSingleClip(clipA, AnimLoopMode::Loop);
        return;
    }
    outDecision.SetCardinalBlend(clipA, clipB, blend);
}

}  // namespace Spark
