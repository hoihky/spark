#pragma once

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/DirectionalLocomotionStrategy.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackDecision.hpp"
#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/animation/locomotion/LocomotionPlaybackApplicator.hpp"
#include "spark/animation/locomotion/PlanarLocomotionKinematicsProvider.hpp"
#include "spark/animation/locomotion/SpeedOnlyLocomotionStrategy.hpp"
#include "spark/animation/locomotion/TurnInPlaceController.hpp"

namespace Spark {

class AnimatorComponent;
class GameObject;
class Skeleton;

/** Locomotion 2.0 driver: speed-only or directional playback with optional turn-in-place. */
class LocomotionController final {
public:
    enum class DriverMode : std::uint8_t {
        SpeedOnly = 0,
        Directional = 1,
    };

    void SetDriverMode(const DriverMode mode) noexcept { driverMode = mode; }
    [[nodiscard]] DriverMode GetDriverMode() const noexcept { return driverMode; }

    void SetWalkSpeedThreshold(const float metersPerSecond) noexcept { walkSpeedThreshold = metersPerSecond; }
    void SetRunSpeedThreshold(const float metersPerSecond) noexcept { runSpeedThreshold = metersPerSecond; }
    void SetCrossfadeDuration(const float seconds) noexcept { applicator.SetCrossfadeDuration(seconds); }

    void SetWishFacingYawRadians(const float radians) noexcept;
    void ClearWishFacingOverride() noexcept { kinematics.ClearWishFacingOverride(); }

    void ConfigureSpeedClips(std::uint32_t idle, std::uint32_t walk, std::uint32_t run) noexcept;
    void ConfigureDirectionalFromSkeleton(const Skeleton& skeleton, const LocomotionClipSet& baseSet) noexcept;

    TurnInPlaceController& GetTurnController() noexcept { return turnController; }

    void Drive(
            GameObject& owner,
            AnimatorComponent& anim,
            bool wantSprint,
            bool useAnalogSpeedOverride,
            float analogSpeedMetersPerSecond) noexcept;

    void NotifyAnimatorAdvanced(const AnimatorComponent& anim) noexcept;

private:
    DriverMode driverMode = DriverMode::SpeedOnly;
    float walkSpeedThreshold = 0.35F;
    float runSpeedThreshold = 2.2F;
    PlanarLocomotionKinematicsProvider kinematics{};
    SpeedOnlyLocomotionStrategy speedStrategy{};
    DirectionalLocomotionStrategy directionalStrategy{};
    TurnInPlaceController turnController{};
    LocomotionPlaybackApplicator applicator{};
    LocomotionPlanarSample sample{};
    LocomotionPlaybackDecision decision{};
};

}  // namespace Spark
