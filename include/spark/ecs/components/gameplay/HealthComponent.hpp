#pragma once

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/gameplay/DamageTypes.hpp"

#include <functional>

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

    [[nodiscard]] float GetMaximum() const noexcept { return maximum; }
    [[nodiscard]] float GetCurrent() const noexcept { return current; }
    [[nodiscard]] bool IsAlive() const noexcept { return current > 0.0F; }

    void SetMaximum(float value) noexcept;
    void SetCurrent(float value) noexcept;
    void ResetToFull() noexcept { current = maximum; }

    /** Returns applied damage after clamping; emits signals when damage > 0. */
    float ApplyDamage(float amount, GameObject* instigator = nullptr);

    void SetOnDeath(std::function<void(GameObject&, GameObject*)> callback) { onDeath = MoveTemp(callback); }

private:
    float maximum = 100.0F;
    float current = 100.0F;
    std::function<void(GameObject&, GameObject*)> onDeath{};
};

}  // namespace Spark
