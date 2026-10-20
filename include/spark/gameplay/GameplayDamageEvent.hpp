#pragma once

#include "spark/ecs/Signal.hpp"

namespace Spark {

class GameObject;
class DamageSignalPayload;

/** Read-only view of a <c>SignalId::DamageApplied</c> dispatch. */
class GameplayDamageEvent final {
public:
    GameplayDamageEvent() noexcept = default;
    explicit GameplayDamageEvent(const DamageSignalPayload& payload) noexcept;

    [[nodiscard]] static bool TryFromSignal(SignalId id, const SignalPayload& payload, GameplayDamageEvent& out) noexcept;

    [[nodiscard]] float GetRequestedAmount() const noexcept { return requestedAmount; }
    [[nodiscard]] float GetAppliedAmount() const noexcept { return appliedAmount; }
    [[nodiscard]] GameObject* GetInstigator() const noexcept { return instigator; }
    [[nodiscard]] GameObject* GetTarget() const noexcept { return target; }

private:
    float requestedAmount = 0.0F;
    float appliedAmount = 0.0F;
    GameObject* instigator = nullptr;
    GameObject* target = nullptr;
};

}  // namespace Spark
