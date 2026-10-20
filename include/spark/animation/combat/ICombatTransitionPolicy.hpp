#pragma once

namespace Spark {

class CombatActionKind;
class CombatPhaseKind;

class ICombatTransitionPolicy {
public:
    virtual ~ICombatTransitionPolicy() = default;

    virtual bool AllowsTransition(
            CombatActionKind activeAction,
            CombatPhaseKind activePhase,
            CombatActionKind requestedAction) const noexcept = 0;
};

}  // namespace Spark
