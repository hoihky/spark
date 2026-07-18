#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

/**
 * Damage routing façade: applies incoming damage to a sibling <c>HealthComponent</c> with optional multiplier.
 * Games can call <c>ApplyDamage</c> on this component instead of reaching into health directly.
 */
class DamageableComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Damageable;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] float GetDamageMultiplier() const noexcept { return damageMultiplier; }
    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerable; }

    void SetDamageMultiplier(float m) noexcept { damageMultiplier = m; }
    void SetInvulnerable(bool v) noexcept { invulnerable = v; }

    float ApplyDamage(float amount, GameObject* instigator = nullptr);

private:
    float damageMultiplier = 1.0F;
    bool invulnerable = false;
};

}  // namespace Spark
