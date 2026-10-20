#include "spark/ecs/components/character/Character3DVisualComponent.hpp"

#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/character/Character3DBodyComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

bool Character3DVisualComponent::ValidateSetup() const noexcept {
    if (bodyObject == nullptr) {
        return false;
    }
    const GameObject* owner = GetOwner();
    if (owner == nullptr) {
        return false;
    }
    if (owner->GetComponent<AnimatorComponent>() == nullptr) {
        return false;
    }
    const Character3DBodyComponent* body = bodyObject->GetComponent<Character3DBodyComponent>();
    if (body == nullptr || body->GetVisualObject() != owner) {
        return false;
    }
    return owner->GetComponent<SkinnedMeshComponent>() != nullptr;
}

}  // namespace Spark
