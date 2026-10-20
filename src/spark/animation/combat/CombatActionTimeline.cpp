#include "spark/animation/combat/CombatActionTimeline.hpp"

#include "spark/animation/combat/CombatPhaseKind.hpp"

namespace Spark {

CombatPhaseKind CombatActionTimeline::ResolvePhase(const float normalizedTime) const noexcept {
    if (normalizedTime >= 1.0F) {
        return CombatPhaseKind{CombatPhaseKind::Value::Complete};
    }
    if (normalizedTime >= activeEnd) {
        return CombatPhaseKind{CombatPhaseKind::Value::Recovery};
    }
    if (normalizedTime >= windupEnd) {
        return CombatPhaseKind{CombatPhaseKind::Value::Active};
    }
    if (normalizedTime > 0.0F) {
        return CombatPhaseKind{CombatPhaseKind::Value::Windup};
    }
    return CombatPhaseKind{CombatPhaseKind::Value::Idle};
}

}  // namespace Spark
