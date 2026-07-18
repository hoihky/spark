#include "spark/ai/GameAiSubsystem.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ai/NavigationSubsystem.hpp"
#include "spark/ai/PerceptionSubsystem.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void SimulateGameAi(GameWorld& world, const FrameTiming& timing, IEngineContext& context) {
    ProcessNavMeshAgents(world);
    ProcessPerceptionSensors(world);
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
