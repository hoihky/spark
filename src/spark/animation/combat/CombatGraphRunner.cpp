#include "spark/animation/combat/CombatGraphRunner.hpp"

namespace Spark {

void CombatGraphRunner::RegisterAction(const CombatActionSpec& spec) noexcept {
    const int slot = spec.GetKind().ToInt();
    if (slot >= 0 && slot < 5) {
        specs[slot] = spec;
        transitionPolicy.RegisterSpec(spec);
    }
}

void CombatGraphRunner::Reset() noexcept {
    activeAction = CombatActionKind{};
    pendingAction = CombatActionKind{};
    activePhase = CombatPhaseKind{};
}

void CombatGraphRunner::ClearActive() noexcept {
    activeAction = CombatActionKind{};
    activePhase = CombatPhaseKind{};
}

void CombatGraphRunner::PromotePendingIfPossible() noexcept {
    if (pendingAction.GetValue() == CombatActionKind::Value::None) {
        return;
    }
    if (!transitionPolicy.AllowsTransition(activeAction, activePhase, pendingAction)) {
        return;
    }
    activeAction = pendingAction;
    pendingAction = CombatActionKind{};
    activePhase = CombatPhaseKind{CombatPhaseKind::Value::Windup};
}

bool CombatGraphRunner::Request(const CombatActionKind action) noexcept {
    if (action.GetValue() == CombatActionKind::Value::None) {
        return false;
    }
    if (action.GetValue() == CombatActionKind::Value::Death) {
        pendingAction = CombatActionKind{};
        activeAction = action;
        activePhase = CombatPhaseKind{CombatPhaseKind::Value::Windup};
        return true;
    }

    if (!HasActiveAction()) {
        activeAction = action;
        activePhase = CombatPhaseKind{CombatPhaseKind::Value::Windup};
        return true;
    }

    if (transitionPolicy.AllowsTransition(activeAction, activePhase, action)) {
        activeAction = action;
        activePhase = CombatPhaseKind{CombatPhaseKind::Value::Windup};
        pendingAction = CombatActionKind{};
        return true;
    }

    if (action.GetValue() != activeAction.GetValue()) {
        pendingAction = action;
    }
    return false;
}

void CombatGraphRunner::AdvanceOverlayPlayback(
        const float normalizedClipTime,
        const bool overlayFinished) noexcept {
    if (!HasActiveAction()) {
        return;
    }
    const CombatActionSpec* spec = FindSpec(activeAction);
    if (spec != nullptr) {
        activePhase = spec->GetTimeline().ResolvePhase(normalizedClipTime);
    }
    if (overlayFinished) {
        ClearActive();
        PromotePendingIfPossible();
    }
}

const CombatActionSpec* CombatGraphRunner::FindSpec(const CombatActionKind action) const noexcept {
    const int slot = action.ToInt();
    if (slot < 0 || slot >= 5 || !specs[slot].IsConfigured()) {
        return nullptr;
    }
    return &specs[slot];
}

}  // namespace Spark
