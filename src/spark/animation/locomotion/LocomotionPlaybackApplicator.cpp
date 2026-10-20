#include "spark/animation/locomotion/LocomotionPlaybackApplicator.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

void LocomotionPlaybackApplicator::ApplySingleClip_(
        AnimatorComponent& anim,
        const std::uint32_t clip,
        const AnimLoopMode loopMode) const noexcept {
    if (clip >= anim.GetClipCount()) {
        return;
    }
    anim.ClearLocomotionBlend();

    const bool sameClip = clip == anim.GetClipIndex();
    const AnimLoopMode prevLoop = anim.GetLoopMode();
    if (sameClip && prevLoop == loopMode && !anim.IsCrossfading() && !anim.IsLocomotionBlending()
            && !(loopMode == AnimLoopMode::Once && anim.IsClipFinished())) {
        return;
    }

    anim.SetLoopMode(loopMode);
    if (!sameClip) {
        anim.SetClipIndexWithCrossfade(clip, crossfadeDuration);
        return;
    }

    if (loopMode == AnimLoopMode::Loop && prevLoop != AnimLoopMode::Loop) {
        if (const SharedPtr<Skeleton>& sk = anim.GetSkeleton()) {
            const float dur = sk->GetClipDuration(clip);
            if (dur > 1.0e-4F && anim.GetTimeSeconds() >= dur - 1.0e-4F) {
                anim.SetTimeSeconds(0.0F);
            }
        }
    } else if (loopMode == AnimLoopMode::Once) {
        anim.RestartCurrentClip();
        anim.SetLoopMode(AnimLoopMode::Once);
    }
}

void LocomotionPlaybackApplicator::Apply(
        AnimatorComponent& anim,
        const LocomotionPlaybackDecision& decision) const noexcept {
    switch (decision.GetMode()) {
        case LocomotionPlaybackDecision::Mode::Idle:
            return;
        case LocomotionPlaybackDecision::Mode::SingleClip:
        case LocomotionPlaybackDecision::Mode::TurnInPlace:
            ApplySingleClip_(anim, decision.GetClipA(), decision.GetLoopMode());
            return;
        case LocomotionPlaybackDecision::Mode::SpeedBlend1D:
        case LocomotionPlaybackDecision::Mode::CardinalDirectionBlend:
            if (decision.GetClipA() >= anim.GetClipCount() || decision.GetClipB() >= anim.GetClipCount()) {
                return;
            }
            anim.SetLoopMode(AnimLoopMode::Loop);
            if (decision.GetClipA() == decision.GetClipB()) {
                ApplySingleClip_(anim, decision.GetClipA(), AnimLoopMode::Loop);
                return;
            }
            anim.SetLocomotionBlend(decision.GetClipA(), decision.GetClipB(), decision.GetBlend01());
            return;
    }
}

}  // namespace Spark
