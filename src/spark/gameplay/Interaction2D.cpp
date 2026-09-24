#include "spark/gameplay/Interaction2D.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/InteractableComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <limits>

namespace Spark {

void ProcessInteractables2D(GameWorld& world, GameObject& instigator, const bool interactPressed) noexcept {
    if (!interactPressed) {
        return;
    }

    const TransformComponent* instigatorTr = instigator.GetComponent<TransformComponent>();
    if (instigatorTr == nullptr) {
        return;
    }
    const Vector3 instigatorPos = instigatorTr->GetLocalTransform().translation;

    InteractableComponent* best = nullptr;
    float bestDist2 = std::numeric_limits<float>::max();

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr || object == &instigator) {
            return;
        }
        auto* interactable = object->GetComponent<InteractableComponent>();
        if (interactable == nullptr || !interactable->CanInteract(instigator)) {
            return;
        }
        const TransformComponent* ownerTr = object->GetComponent<TransformComponent>();
        if (ownerTr == nullptr) {
            return;
        }
        const Vector3 ownerPos = ownerTr->GetLocalTransform().translation;
        const float dx = ownerPos.x - instigatorPos.x;
        const float dy = ownerPos.y - instigatorPos.y;
        const float dist2 = dx * dx + dy * dy;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best = interactable;
        }
    });

    if (best != nullptr) {
        best->TryInteract(instigator);
    }
}

}  // namespace Spark
