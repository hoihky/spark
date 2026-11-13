#include "spark/ai/AiSubsystem.hpp"

#include "spark/ai/NavigationSubsystem.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/ai/PerceptionSubsystem.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void AiSubsystem::ProcessGridNav2D(GameWorld& world, float deltaTimeSeconds) noexcept {
    ProcessGridNavAgents2D(world, deltaTimeSeconds);
}

void AiSubsystem::ProcessNavMesh(GameWorld& world) noexcept {
    ProcessNavMeshAgents(world);
}

void AiSubsystem::ProcessPerception(GameWorld& world) noexcept {
    ProcessPerceptionSensors(world);
}

void AiSubsystem::RunFrame(GameWorld& world, const FrameTiming& timing, IEngineContext& context) {
    ProcessGridNav2D(world, timing.deltaTimeSeconds);
    ProcessNavMesh(world);
    ProcessPerception(world);
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        AiAgentComponent* agent = o->GetComponent<AiAgentComponent>();
        if (agent == nullptr || !agent->IsEnabled()) {
            return;
        }
        agent->SubsystemTick(timing, *o, context);
    });
}

}  // namespace Spark
