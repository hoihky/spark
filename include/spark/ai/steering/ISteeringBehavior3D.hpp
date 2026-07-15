#pragma once

#include "spark/ai/steering/SteeringEnvironment3D.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class AiBlackboard;

class ISteeringBehavior3D {
public:
    virtual ~ISteeringBehavior3D() = default;

    virtual Vector3 Compute(
            const Vector3& position,
            const Vector3& velocity,
            const SteeringEnvironment3D& env,
            AiBlackboard& board) const = 0;
};

}  // namespace Spark
