#include "spark/gameplay/GameplayInteractionProcessor.hpp"

#include "spark/ecs/components/gameplay/InteractableComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayInteractionCandidate.hpp"
#include "spark/gameplay/GameplayInteractionQuery.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

bool GameplayInteractionProcessor::ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept {
    const GameplayInteractionCandidate candidate = query.FindBest(world, instigator);
    if (!candidate.IsValid()) {
        return false;
    }
    return candidate.GetInteractable()->TryInteract(instigator);
}

}  // namespace Spark
