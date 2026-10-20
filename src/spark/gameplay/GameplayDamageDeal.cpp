#include "spark/gameplay/GameplayDamageDeal.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayDamageApplier.hpp"
#include "spark/gameplay/GameplayTeamPolicy.hpp"

namespace Spark {

float GameplayDamageDeal::Apply(const float rawDamage) noexcept {
    if (target == nullptr) {
        return 0.0F;
    }
    if (teamPolicy != nullptr && instigator != nullptr && !teamPolicy->AllowsHostileAction(instigator, *target)) {
        return 0.0F;
    }
    return GameplayDamageApplier::ApplyToHealthHierarchy(*target, instigator, rawDamage, hurtboxNotifier);
}

}  // namespace Spark
