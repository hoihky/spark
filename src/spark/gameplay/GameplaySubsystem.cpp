#include "spark/gameplay/GameplaySubsystem.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/gameplay/GameplayInteractionProcessor.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

namespace {

GameplayInteractionProcessor& GetSharedInteractProcessor() noexcept {
    static GameplayInteractionProcessor processor{};
    return processor;
}

}  // namespace

void GameplaySubsystem::AfterPhysics2D(GameWorld& world) noexcept {
    PickupComponent::ProcessDeferredDestroys(world);
}

bool GameplaySubsystem::ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept {
    return GetSharedInteractProcessor().ProcessInteractPress(world, instigator);
}

}  // namespace Spark
