#include "spark/character/KinematicCharacterMotor.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <exception>

namespace Spark {

void KinematicCharacterMotor::Bind(
        CharacterController3DComponent* controllerIn,
        TransformComponent* transformIn) noexcept {
    controller = controllerIn;
    transform = transformIn;
}

void KinematicCharacterMotor::SetGameplayMoveInput(const Vector3& worldVelocity) {
    if (controller == nullptr) {
        return;
    }
    controller->SetMoveInput(worldVelocity);
}

void KinematicCharacterMotor::ApplyWorldDelta(const Vector3& worldDelta) {
    if (transform == nullptr) {
        return;
    }
    const Vector3 pos = transform->GetLocalTransform().translation;
    transform->SetTranslation(pos + worldDelta);
}

CharacterLocomotionState KinematicCharacterMotor::QueryState() const {
    CharacterLocomotionState state{};
    if (controller != nullptr) {
        state.velocity = controller->GetVelocity();
        state.grounded = controller->IsGrounded();
    }
    return state;
}

GameObject& KinematicCharacterMotor::GetBodyRoot() {
    if (controller != nullptr && controller->GetOwner() != nullptr) {
        return *controller->GetOwner();
    }
    if (transform != nullptr && transform->GetOwner() != nullptr) {
        return *transform->GetOwner();
    }
    std::terminate();
}

const GameObject& KinematicCharacterMotor::GetBodyRoot() const {
    return const_cast<KinematicCharacterMotor*>(this)->GetBodyRoot();
}

}  // namespace Spark
