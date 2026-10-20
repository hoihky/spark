#include "spark/ecs/components/character/Character3DBodyComponent.hpp"

#include "spark/animation/IRootMotionApplicator.hpp"
#include "spark/character/CharacterRootMotionApplicator.hpp"
#include "spark/character/CharacterSimulationPipeline.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/animation/RootMotionComponent.hpp"
#include "spark/ecs/components/character/Character3DVisualComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

void Character3DBodyComponent::OnAttach(GameObject& owner) {
    controller = owner.GetComponent<CharacterController3DComponent>();
    bodyTransform = owner.GetComponent<TransformComponent>();
    motor.Bind(controller, bodyTransform);
}

void Character3DBodyComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& /*owner*/,
        IEngineContext& /*context*/) {
    ApplyPostAnimationMotion(*this);
}

void Character3DBodyComponent::WireRootMotion(RootMotionComponent& rootMotion) {
    auto applicator = MakeUnique<CharacterRootMotionApplicator>(arbitrator);
    rootMotion.SetCustomApplicator(UniquePtr<IRootMotionApplicator>(applicator.Release()));
}

bool Character3DBodyComponent::ValidateHierarchy() const noexcept {
    if (controller == nullptr || bodyTransform == nullptr) {
        return false;
    }
    if (visualObject == nullptr) {
        return false;
    }
    const Character3DVisualComponent* visual = visualObject->GetComponent<Character3DVisualComponent>();
    if (visual == nullptr || visual->GetBodyObject() == nullptr) {
        return false;
    }
    return visualObject->GetComponent<AnimatorComponent>() != nullptr;
}

}  // namespace Spark
