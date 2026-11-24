#include "spark/gameplay/GameplaySubsystem.hpp"

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
    world.FlushPendingDestroyGameObjects();
}

bool GameplaySubsystem::ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept {
    return GetSharedInteractProcessor().ProcessInteractPress(world, instigator);
}

}  // namespace Spark
