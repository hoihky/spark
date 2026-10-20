#include "spark/gameplay/GameplayTeamPolicy.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayTeamAffiliation.hpp"
#include "spark/gameplay/GameplayTeamId.hpp"
#include "spark/gameplay/GameplayTeamRelation.hpp"

namespace Spark {

GameplayTeamRelation GameplayTeamPolicy::Classify(
        const GameObject* source,
        const GameObject& target) const noexcept {
    if (source == &target) {
        return GameplayTeamRelation{GameplayTeamRelation::Value::Self};
    }
    const GameplayTeamId sourceTeam =
            source != nullptr ? GameplayTeamAffiliation::Resolve(*source) : GameplayTeamId{};
    const GameplayTeamId targetTeam = GameplayTeamAffiliation::Resolve(target);
    if (sourceTeam.IsNeutral() || targetTeam.IsNeutral()) {
        return GameplayTeamRelation{GameplayTeamRelation::Value::Neutral};
    }
    if (sourceTeam == targetTeam) {
        return GameplayTeamRelation{GameplayTeamRelation::Value::Ally};
    }
    return GameplayTeamRelation{GameplayTeamRelation::Value::Enemy};
}

bool GameplayTeamPolicy::AllowsHostileAction(const GameObject* source, const GameObject& target) const noexcept {
    const GameplayTeamRelation relation = Classify(source, target);
    switch (relation.GetValue()) {
        case GameplayTeamRelation::Value::Self:
            return false;
        case GameplayTeamRelation::Value::Ally:
            return allowFriendlyFire;
        case GameplayTeamRelation::Value::Enemy:
            return true;
        case GameplayTeamRelation::Value::Neutral:
            return true;
    }
    return true;
}

}  // namespace Spark
