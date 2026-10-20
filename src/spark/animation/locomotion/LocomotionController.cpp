#include "spark/animation/locomotion/LocomotionController.hpp"

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

void LocomotionController::SetWishFacingYawRadians(const float radians) noexcept {
    kinematics.SetWishFacingYawRadians(radians);
}

void LocomotionController::ConfigureSpeedClips(
        const std::uint32_t idle,
        const std::uint32_t walk,
        const std::uint32_t run) noexcept {
    speedStrategy.SetIdleClip(idle);
    speedStrategy.SetWalkClip(walk);
    speedStrategy.SetRunClip(run);
}

void LocomotionController::ConfigureDirectionalFromSkeleton(
        const Skeleton& skeleton,
        const LocomotionClipSet& baseSet) noexcept {
    directionalStrategy.GetCatalog().ResolveFromSkeleton(skeleton, baseSet);
}

void LocomotionController::Drive(
        GameObject& owner,
        AnimatorComponent& anim,
        const bool wantSprint,
        const bool useAnalogSpeedOverride,
        const float analogSpeedMetersPerSecond) noexcept {
    kinematics.FillSample(owner, sample);

    if (driverMode == DriverMode::Directional
            && turnController.TryBuildTurnPlayback(
                    sample, directionalStrategy.GetCatalog(), anim, decision)) {
        applicator.Apply(anim, decision);
        return;
    }

    if (driverMode == DriverMode::Directional
            && directionalStrategy.GetCatalog().HasDirectionalLocomotion(anim)) {
        directionalStrategy.BuildPlayback(
                sample, anim, walkSpeedThreshold, runSpeedThreshold, wantSprint, decision);
        applicator.Apply(anim, decision);
        return;
    }

    if (useAnalogSpeedOverride) {
        sample.SetVelocityWorldX(0.0F);
        sample.SetVelocityWorldZ(analogSpeedMetersPerSecond);
    }

    speedStrategy.BuildPlayback(
            sample, anim, walkSpeedThreshold, runSpeedThreshold, wantSprint, decision);
    applicator.Apply(anim, decision);
}

void LocomotionController::NotifyAnimatorAdvanced(const AnimatorComponent& anim) noexcept {
    turnController.NotifyAnimatorAdvanced(anim);
}

}  // namespace Spark
