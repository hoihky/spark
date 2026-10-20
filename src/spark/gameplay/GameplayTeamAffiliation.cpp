#include "spark/gameplay/GameplayTeamAffiliation.hpp"

#include "spark/ecs/components/gameplay/GameplayTeamComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayTeamId.hpp"

namespace Spark {

GameplayTeamId GameplayTeamAffiliation::Resolve(const GameObject& object) noexcept {
    for (const GameObject* node = &object; node != nullptr; node = node->GetParent()) {
        if (const GameplayTeamComponent* team = node->GetComponent<GameplayTeamComponent>()) {
            return team->GetGameplayTeam();
        }
    }
    return GameplayTeamId{};
}

}  // namespace Spark
