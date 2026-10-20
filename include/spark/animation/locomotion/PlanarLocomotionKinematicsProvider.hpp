#pragma once

#include "spark/animation/locomotion/ILocomotionKinematicsProvider.hpp"

namespace Spark {

/** Reads body yaw from the owner hierarchy and velocity from controller / rigidbody parents. */
class PlanarLocomotionKinematicsProvider final : public ILocomotionKinematicsProvider {
public:
    void SetWishFacingYawRadians(const float radians) noexcept { wishFacingYaw = radians; }
    void ClearWishFacingOverride() noexcept { hasWishFacingOverride = false; }

    void FillSample(const GameObject& owner, LocomotionPlanarSample& outSample) const noexcept override;

private:
    float wishFacingYaw = 0.0F;
    bool hasWishFacingOverride = false;
};

}  // namespace Spark
