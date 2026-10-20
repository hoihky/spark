#pragma once

#include "spark/animation/combat/CombatActionSpec.hpp"
#include "spark/animation/combat/ICombatTransitionPolicy.hpp"

namespace Spark {

/** Priority + phase gates (e.g. stagger only during Active/Recovery). */
class CombatGraphTransitionPolicy final : public ICombatTransitionPolicy {
public:
    void RegisterSpec(const CombatActionSpec& spec) noexcept;

    bool AllowsTransition(
            CombatActionKind activeAction,
            CombatPhaseKind activePhase,
            CombatActionKind requestedAction) const noexcept override;

private:
    [[nodiscard]] int PriorityFor(CombatActionKind kind) const noexcept;

    CombatActionSpec specs[5]{};
};

}  // namespace Spark
