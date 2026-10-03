#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_damage_multiplier)
    [[nodiscard]] float GetDamageMultiplier() const noexcept { return damageMultiplier; }
    SPARK_SCRIPT_BIND(is_invulnerable)
    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerable; }

    SPARK_SCRIPT_BIND(set_damage_multiplier)
    void SetDamageMultiplier(float m) noexcept { damageMultiplier = m; }
    SPARK_SCRIPT_BIND(set_invulnerable)
    void SetInvulnerable(bool v) noexcept { invulnerable = v; }

    SPARK_SCRIPT_BIND(get_apply_damage)
    float ApplyDamage(float amount, GameObject* instigator = nullptr);

private:
    float damageMultiplier = 1.0F;
    bool invulnerable = false;
};

}  // namespace Spark
