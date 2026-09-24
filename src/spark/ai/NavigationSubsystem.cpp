#include "spark/ai/NavigationSubsystem.hpp"

#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void ProcessGridNavAgents2D(GameWorld& world, const float deltaTimeSeconds) noexcept {
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        GridNavAgent2DComponent* nav = o->GetComponent<GridNavAgent2DComponent>();
        if (nav == nullptr || !nav->IsEnabled()) {
            return;
        }
        nav->SubsystemTick(*o, deltaTimeSeconds);
    });
}

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
