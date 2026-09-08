#include "spark/animation/IRootMotionApplicator.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

void TransformRootMotionApplicator::Apply(GameObject& target, const Vector3& worldDelta) {
    TransformComponent* transform = target.GetComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }
    const Vector3 pos = transform->GetLocalTransform().translation;
    transform->SetTranslation(pos + worldDelta);
}

}  // namespace Spark
