#include "spark/ecs/components/gameplay/HealthComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

#include <algorithm>

namespace Spark {

void HealthComponent::SetMaximum(const float value) noexcept {
    maximum = std::max(value, 0.0F);
    current = std::min(current, maximum);
}

void HealthComponent::SetCurrent(const float value) noexcept {
    current = std::clamp(value, 0.0F, maximum);
}

float HealthComponent::ApplyDamage(const float amount, GameObject* instigator) {
    if (amount <= 0.0F || current <= 0.0F) {
        return 0.0F;
    }
    const float applied = std::min(amount, current);
    current -= applied;
    GameObject* owner = GetOwner();
    if (owner == nullptr) {
        return applied;
    }
    DamageSignalPayload payload{};
    payload.amount = amount;
    payload.applied = applied;
    payload.instigator = instigator;
    payload.target = owner;
    SignalPayload signal{};
    signal.ptr = &payload;
    owner->EmitSignal(SignalId::DamageApplied, signal, this);
    if (current <= 0.0F) {
        SignalPayload death{};
        death.ptr = instigator;
        owner->EmitSignal(SignalId::Died, death, this);
        if (onDeath) {
            onDeath(*owner, instigator);
        }
    }
    return applied;
}

}  // namespace Spark
