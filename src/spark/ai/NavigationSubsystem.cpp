#include "spark/ai/NavigationSubsystem.hpp"

#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void ProcessNavMeshAgents(GameWorld& world) noexcept {
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        NavMeshAgentComponent* nav = o->GetComponent<NavMeshAgentComponent>();
        if (nav == nullptr || !nav->IsEnabled()) {
            return;
        }
        nav->SubsystemTick(*o);
    });
}

}  // namespace Spark
