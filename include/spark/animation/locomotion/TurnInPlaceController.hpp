#pragma once

#include "spark/animation/locomotion/DirectionalLocomotionClipCatalog.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class LocomotionPlanarSample;
class LocomotionPlaybackDecision;

/** Plays a one-shot turn clip when planar speed is low but facing error is large. */
class TurnInPlaceController final {
public:
    void SetEnabled(const bool value) noexcept { enabled = value; }
    void SetMaxPlanarSpeed(const float metersPerSecond) noexcept { maxPlanarSpeed = metersPerSecond; }
    void SetMinFacingErrorRadians(const float radians) noexcept { minFacingError = radians; }

    void Reset() noexcept { turningActive = false; }

    /** When true, locomotion strategies should not override base playback this frame. */
    [[nodiscard]] bool IsTurning() const noexcept { return turningActive; }

    bool TryBuildTurnPlayback(
            const LocomotionPlanarSample& sample,
            const DirectionalLocomotionClipCatalog& catalog,
            const AnimatorComponent& anim,
            LocomotionPlaybackDecision& outDecision) noexcept;

    void NotifyAnimatorAdvanced(const AnimatorComponent& anim) noexcept;

private:
    bool enabled = true;
    float maxPlanarSpeed = 0.2F;
    float minFacingError = 0.45F;
    bool turningActive = false;
    std::uint32_t activeTurnClip = kInvalidAnimClipIndex;
};

}  // namespace Spark
