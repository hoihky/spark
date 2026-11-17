#include "spark/animation/character/CharacterRootMotionApplicator.hpp"

#include "spark/animation/character/MotionArbitrator.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

CharacterRootMotionApplicator::CharacterRootMotionApplicator(MotionArbitrator& arbitratorIn) noexcept
        : arbitrator(arbitratorIn) {}

void CharacterRootMotionApplicator::Apply(GameObject& /*target*/, const Vector3& worldDelta) {
    arbitrator.AccumulateRootMotionDelta(worldDelta);
}

}  // namespace Spark
