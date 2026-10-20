#include "spark/gameplay/GameplayInteractionQuery.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/InteractableComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayInteractionCandidate.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <limits>

namespace Spark {

GameplayInteractionCandidate GameplayInteractionQuery::FindBest(
        GameWorld& world,
        GameObject& instigator) const noexcept {
    const TransformComponent* instigatorTr = instigator.GetComponent<TransformComponent>();
    if (instigatorTr == nullptr) {
        return {};
    }
    const Vector3 instigatorPos = instigatorTr->GetLocalTransform().translation;

    InteractableComponent* bestInteractable = nullptr;
    GameObject* bestOwner = nullptr;
    float bestDist2 = std::numeric_limits<float>::max();

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr || object == &instigator) {
            return;
        }
        InteractableComponent* interactable = object->GetComponent<InteractableComponent>();
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
        const float dz = ownerPos.z - instigatorPos.z;
        const float dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            bestInteractable = interactable;
            bestOwner = object;
        }
    });

    return GameplayInteractionCandidate{bestInteractable, bestOwner, bestDist2};
}

}  // namespace Spark
