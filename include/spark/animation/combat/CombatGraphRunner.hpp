#pragma once

#include "spark/animation/combat/CombatActionKind.hpp"
#include "spark/animation/combat/CombatActionSpec.hpp"
#include "spark/animation/combat/CombatGraphTransitionPolicy.hpp"
#include "spark/animation/combat/CombatPhaseKind.hpp"

namespace Spark {

/** Lightweight combat state machine with phase-aware cancel rules. */
class CombatGraphRunner final {
public:
    void RegisterAction(const CombatActionSpec& spec) noexcept;

    void Reset() noexcept;

    /** Returns true when the request is accepted (queued or applied immediately). */
    bool Request(const CombatActionKind action) noexcept;

    void AdvanceOverlayPlayback(const float normalizedClipTime, const bool overlayFinished) noexcept;

    [[nodiscard]] CombatActionKind GetActiveAction() const noexcept { return activeAction; }
    [[nodiscard]] CombatPhaseKind GetActivePhase() const noexcept { return activePhase; }
    [[nodiscard]] CombatActionKind GetPendingAction() const noexcept { return pendingAction; }
    [[nodiscard]] bool HasActiveAction() const noexcept {
        return activeAction.GetValue() != CombatActionKind::Value::None;
    }

    [[nodiscard]] const CombatActionSpec* FindSpec(const CombatActionKind action) const noexcept;

private:
    void PromotePendingIfPossible() noexcept;
    void ClearActive() noexcept;

    CombatGraphTransitionPolicy transitionPolicy{};
    CombatActionSpec specs[5]{};
    CombatActionKind activeAction{};
    CombatActionKind pendingAction{};
    CombatPhaseKind activePhase{};
};

}  // namespace Spark
