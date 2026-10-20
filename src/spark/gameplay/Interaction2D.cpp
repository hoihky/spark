#include "spark/gameplay/Interaction2D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayInteractionProcessor.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void ProcessInteractables2D(GameWorld& world, GameObject& instigator, const bool interactPressed) noexcept {
    if (!interactPressed) {
        return;
    }
    static GameplayInteractionProcessor processor{};
    (void)processor.ProcessInteractPress(world, instigator);
}

}  // namespace Spark
