#pragma once

#include "spark/animation/IRootMotionApplicator.hpp"

namespace Spark {

class MotionArbitrator;

/** Feeds root-motion deltas into a <c>MotionArbitrator</c> instead of moving the transform directly. */
class CharacterRootMotionApplicator final : public IRootMotionApplicator {
public:
    explicit CharacterRootMotionApplicator(MotionArbitrator& arbitratorIn) noexcept;

    void Apply(GameObject& target, const Vector3& worldDelta) override;

private:
    MotionArbitrator& arbitrator;
};

}  // namespace Spark
