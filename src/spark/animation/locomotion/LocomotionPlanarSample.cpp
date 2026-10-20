#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float WrapPi(const float radians) noexcept {
    float a = radians;
    constexpr float kTwoPi = 6.283185307179586F;
    while (a > 3.141592653589793F) {
        a -= kTwoPi;
    }
    while (a < -3.141592653589793F) {
        a += kTwoPi;
    }
    return a;
}

[[nodiscard]] float SignedAngleDelta(const float fromYaw, const float toYaw) noexcept {
    return WrapPi(toYaw - fromYaw);
}

}  // namespace

float LocomotionPlanarSample::GetPlanarSpeed() const noexcept {
    return std::sqrt(velocityWorldX * velocityWorldX + velocityWorldZ * velocityWorldZ);
}

float LocomotionPlanarSample::GetSignedVelocityYawRelativeToBody() const noexcept {
    const float speed = GetPlanarSpeed();
    if (speed <= 1.0e-4F) {
        return 0.0F;
    }
    const float velYaw = std::atan2(velocityWorldX, velocityWorldZ);
    return SignedAngleDelta(bodyFacingYaw, velYaw);
}

float LocomotionPlanarSample::GetSignedWishFacingDelta() const noexcept {
    return SignedAngleDelta(bodyFacingYaw, wishFacingYaw);
}

}  // namespace Spark
