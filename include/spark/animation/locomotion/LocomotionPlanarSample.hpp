#pragma once

namespace Spark {

/** Horizontal locomotion kinematics in world space (Y ignored for gameplay). */
class LocomotionPlanarSample final {
public:
    LocomotionPlanarSample() noexcept = default;

    void SetBodyFacingYawRadians(const float radians) noexcept { bodyFacingYaw = radians; }
    void SetWishFacingYawRadians(const float radians) noexcept { wishFacingYaw = radians; }
    void SetVelocityWorldX(const float metersPerSecond) noexcept { velocityWorldX = metersPerSecond; }
    void SetVelocityWorldZ(const float metersPerSecond) noexcept { velocityWorldZ = metersPerSecond; }

    [[nodiscard]] float GetBodyFacingYawRadians() const noexcept { return bodyFacingYaw; }
    [[nodiscard]] float GetWishFacingYawRadians() const noexcept { return wishFacingYaw; }
    [[nodiscard]] float GetVelocityWorldX() const noexcept { return velocityWorldX; }
    [[nodiscard]] float GetVelocityWorldZ() const noexcept { return velocityWorldZ; }

    [[nodiscard]] float GetPlanarSpeed() const noexcept;
    [[nodiscard]] float GetSignedVelocityYawRelativeToBody() const noexcept;
    [[nodiscard]] float GetSignedWishFacingDelta() const noexcept;

private:
    float bodyFacingYaw = 0.0F;
    float wishFacingYaw = 0.0F;
    float velocityWorldX = 0.0F;
    float velocityWorldZ = 0.0F;
};

}  // namespace Spark
