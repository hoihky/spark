#pragma once

namespace Spark {

class GameObject;
class LocomotionPlanarSample;

/** Supplies planar locomotion samples for animation drivers. */
class ILocomotionKinematicsProvider {
public:
    virtual ~ILocomotionKinematicsProvider() = default;

    virtual void FillSample(const GameObject& owner, LocomotionPlanarSample& outSample) const noexcept = 0;
};

}  // namespace Spark
