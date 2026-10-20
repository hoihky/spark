#include "spark/gameplay/GameplayDamageEvent.hpp"

#include "spark/gameplay/DamageTypes.hpp"

namespace Spark {

GameplayDamageEvent::GameplayDamageEvent(const DamageSignalPayload& payload) noexcept
    : requestedAmount(payload.amount),
      appliedAmount(payload.applied),
      instigator(payload.instigator),
      target(payload.target) {}

bool GameplayDamageEvent::TryFromSignal(
        const SignalId id,
        const SignalPayload& payload,
        GameplayDamageEvent& out) noexcept {
    if (id != SignalId::DamageApplied || payload.ptr == nullptr) {
        return false;
    }
    const DamageSignalPayload* damage = static_cast<const DamageSignalPayload*>(payload.ptr);
    out = GameplayDamageEvent{*damage};
    return true;
}

}  // namespace Spark
