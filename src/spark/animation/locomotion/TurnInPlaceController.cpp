#include "spark/animation/locomotion/TurnInPlaceController.hpp"

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"

#include <cmath>

namespace Spark {

bool TurnInPlaceController::TryBuildTurnPlayback(
        const LocomotionPlanarSample& sample,
        const DirectionalLocomotionClipCatalog& catalog,
        const AnimatorComponent& anim,
        LocomotionPlaybackDecision& outDecision) noexcept {
    if (!enabled) {
        turningActive = false;
        return false;
    }

    if (turningActive) {
        if (activeTurnClip < anim.GetClipCount()) {
            outDecision.SetTurnInPlace(activeTurnClip);
            return true;
        }
        turningActive = false;
    }

    if (sample.GetPlanarSpeed() > maxPlanarSpeed) {
        return false;
    }

    const float facingError = sample.GetSignedWishFacingDelta();
    if (std::fabs(facingError) < minFacingError) {
        return false;
    }

    const std::uint32_t wantClip =
            (facingError > 0.0F) ? catalog.GetTurnRightClip() : catalog.GetTurnLeftClip();
    if (wantClip == kInvalidAnimClipIndex || wantClip >= anim.GetClipCount()) {
        return false;
    }

    activeTurnClip = wantClip;
    turningActive = true;
    outDecision.SetTurnInPlace(activeTurnClip);
    return true;
}

void TurnInPlaceController::NotifyAnimatorAdvanced(const AnimatorComponent& anim) noexcept {
    if (!turningActive) {
        return;
    }
    if (anim.GetClipIndex() == activeTurnClip && anim.IsClipFinished()) {
        turningActive = false;
        activeTurnClip = kInvalidAnimClipIndex;
    }
}

}  // namespace Spark
