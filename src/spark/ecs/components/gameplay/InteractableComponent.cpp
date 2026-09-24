#include "spark/ecs/components/gameplay/InteractableComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

void InteractableComponent::SetPromptText(const char* text) noexcept {
    promptText = Utf8String(text != nullptr ? text : "Interact");
}

void InteractableComponent::SetInteractionRadius(const float radius) noexcept {
    interactionRadius = (radius > 0.01F) ? radius : 1.25F;
}

void InteractableComponent::SetRequiredInstigatorTag(const char* tag) noexcept {
    requiredInstigatorTag = Utf8String(tag != nullptr ? tag : "");
}

bool InteractableComponent::IsInstigatorInRange(const GameObject& instigator) const noexcept {
    const GameObject* owner = GetOwner();
    if (owner == nullptr) {
        return false;
    }
    if (const auto* volume = owner->GetComponent<TriggerVolume2DComponent>()) {
        return volume->IsOverlapping(instigator);
    }
    const TransformComponent* ownerTr = owner->GetComponent<TransformComponent>();
    const TransformComponent* instigatorTr = instigator.GetComponent<TransformComponent>();
    if (ownerTr == nullptr || instigatorTr == nullptr) {
        return false;
    }
    const Vector3 ownerPos = ownerTr->GetLocalTransform().translation;
    const Vector3 instigatorPos = instigatorTr->GetLocalTransform().translation;
    const float dx = instigatorPos.x - ownerPos.x;
    const float dy = instigatorPos.y - ownerPos.y;
    const float r2 = interactionRadius * interactionRadius;
    return dx * dx + dy * dy <= r2;
}

bool InteractableComponent::CanInteract(const GameObject& instigator) const noexcept {
    if (!enabled) {
        return false;
    }
    if (!requiredInstigatorTag.IsEmpty()) {
        if (std::strcmp(instigator.GetTag().CStr(), requiredInstigatorTag.CStr()) != 0) {
            return false;
        }
    }
    return IsInstigatorInRange(instigator);
}

bool InteractableComponent::TryInteract(GameObject& instigator) noexcept {
    if (!CanInteract(instigator)) {
        return false;
    }
    if (onInteract) {
        onInteract(instigator);
    }
    return true;
}

}  // namespace Spark
