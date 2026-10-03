#pragma once

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/core/Function.hpp"
#include "spark/gameplay/DamageTypes.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Hit-point pool on a GameObject. Apply damage via <c>ApplyDamage</c> or sibling <c>DamageableComponent</c>.
 * Emits <c>SignalId::DamageApplied</c> and <c>SignalId::Died</c> to sibling components.
 */
class HealthComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Health;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit HealthComponent(float maxHealth = 100.0F) noexcept : maximum(maxHealth), current(maxHealth) {}

    SPARK_SCRIPT_BIND(get_maximum)
    [[nodiscard]] float GetMaximum() const noexcept { return maximum; }
    SPARK_SCRIPT_BIND(get_current)
    [[nodiscard]] float GetCurrent() const noexcept { return current; }
    SPARK_SCRIPT_BIND(is_alive)
    [[nodiscard]] bool IsAlive() const noexcept { return current > 0.0F; }

    SPARK_SCRIPT_BIND(set_maximum)
    void SetMaximum(float value) noexcept;
    SPARK_SCRIPT_BIND(set_current)
    void SetCurrent(float value) noexcept;
    SPARK_SCRIPT_BIND(reset_to_full)
    void ResetToFull() noexcept { current = maximum; }

    /** Returns applied damage after clamping; emits signals when damage > 0. */
    SPARK_SCRIPT_BIND(get_apply_damage)
    float ApplyDamage(float amount, GameObject* instigator = nullptr);

    void SetOnDeath(Function<void(GameObject&, GameObject*)> callback) { onDeath = MoveTemp(callback); }

private:
    float maximum = 100.0F;
    float current = 100.0F;
    Function<void(GameObject&, GameObject*)> onDeath{};
};

}  // namespace Spark
