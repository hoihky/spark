#include "spark/animation/combat/CombatGraphTransitionPolicy.hpp"

#include "spark/animation/combat/CombatActionKind.hpp"
#include "spark/animation/combat/CombatPhaseKind.hpp"

namespace Spark {

void CombatGraphTransitionPolicy::RegisterSpec(const CombatActionSpec& spec) noexcept {
    const int slot = spec.GetKind().ToInt();
    if (slot >= 0 && slot < 5) {
        specs[slot] = spec;
    }
}

int CombatGraphTransitionPolicy::PriorityFor(const CombatActionKind kind) const noexcept {
    const int slot = kind.ToInt();
    if (slot >= 0 && slot < 5 && specs[slot].IsConfigured()) {
        return specs[slot].GetInterruptPriority();
    }
    return 0;
}

bool CombatGraphTransitionPolicy::AllowsTransition(
        const CombatActionKind activeAction,
        const CombatPhaseKind activePhase,
        const CombatActionKind requestedAction) const noexcept {
    if (requestedAction.GetValue() == CombatActionKind::Value::None) {
        return false;
    }
    if (requestedAction.GetValue() == CombatActionKind::Value::Death) {
        return true;
    }
    if (activeAction.GetValue() == CombatActionKind::Value::None) {
        return true;
    }
    if (activeAction.GetValue() == requestedAction.GetValue()) {
        return false;
    }

    const int requestedPriority = PriorityFor(requestedAction);
    const int activePriority = PriorityFor(activeAction);
    if (requestedPriority <= activePriority) {
        return false;
    }

    const auto phase = activePhase.GetValue();
    if (requestedAction.GetValue() == CombatActionKind::Value::Hurt) {
        return phase != CombatPhaseKind::Value::Windup;
    }
    if (requestedAction.GetValue() == CombatActionKind::Value::Stagger) {
        return phase == CombatPhaseKind::Value::Active || phase == CombatPhaseKind::Value::Recovery;
    }
    if (requestedAction.GetValue() == CombatActionKind::Value::Attack) {
        return phase == CombatPhaseKind::Value::Recovery || phase == CombatPhaseKind::Value::Complete;
    }
    return requestedPriority > activePriority;
}

}  // namespace Spark
