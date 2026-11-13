#include "spark/scene/core/world/WorldGameplay.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplaySubsystem.hpp"

namespace Spark {

void WorldGameplay::AfterPhysics2D() noexcept {
    GameplaySubsystem::AfterPhysics2D(world_);
}

bool WorldGameplay::InteractPress(GameObject& instigator) noexcept {
    return GameplaySubsystem::ProcessInteractPress(world_, instigator);
}

void WorldGameplay::InteractIfPressed2D(GameObject& instigator, const bool pressed) noexcept {
    if (!pressed) {
        return;
    }
    (void)InteractPress(instigator);
}

}  // namespace Spark
