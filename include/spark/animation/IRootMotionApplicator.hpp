#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

/** Applies a world-space root-motion translation delta to gameplay (transform, motor, etc.). */
class IRootMotionApplicator {
public:
    virtual ~IRootMotionApplicator() = default;

    virtual void Apply(GameObject& target, const Vector3& worldDelta) = 0;
};

/** Adds <c>worldDelta</c> to the target <c>TransformComponent</c> world translation. */
class TransformRootMotionApplicator final : public IRootMotionApplicator {
public:
    void Apply(GameObject& target, const Vector3& worldDelta) override;
};

}  // namespace Spark
