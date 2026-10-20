#pragma once

#include "spark/gameplay/GameplayDamageApplier.hpp"

namespace Spark {

class GameObject;
class GameplayTeamPolicy;

/**
 * Gameplay-kit entry point for applying damage with optional team filtering.
 * Combat systems (<c>Combat2DTarget</c>, <c>Combat3DTarget</c>) build on the same applier.
 */
class GameplayDamageDeal final {
public:
    explicit GameplayDamageDeal(GameObject& target) noexcept : target(&target) {}

    void SetInstigator(GameObject* source) noexcept { instigator = source; }
    void SetTeamPolicy(const GameplayTeamPolicy* policy) noexcept { teamPolicy = policy; }
    void SetHurtboxNotifier(GameplayDamageApplier::IHurtboxNotifier* notifier) noexcept {
        hurtboxNotifier = notifier;
    }

    [[nodiscard]] float Apply(float rawDamage) noexcept;

private:
    GameObject* target = nullptr;
    GameObject* instigator = nullptr;
    const GameplayTeamPolicy* teamPolicy = nullptr;
    GameplayDamageApplier::IHurtboxNotifier* hurtboxNotifier = nullptr;
};

}  // namespace Spark
