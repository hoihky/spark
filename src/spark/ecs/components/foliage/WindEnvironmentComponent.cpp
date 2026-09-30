#include "spark/ecs/components/foliage/WindEnvironmentComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/foliage/WindSubsystem.hpp"

namespace Spark {

void WindEnvironmentComponent::OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) {
    (void)context;
    GameWorld& world = owner.GetWorld();
    world.GetWindSubsystem().ApplyEnvironmentSettings(settings);
    world.GetWindSubsystem().AdvanceSimulation(timing.deltaTimeSeconds);
}

}  // namespace Spark
