#pragma once

namespace Spark {

class CombatPhaseKind;

/** Normalized [0,1] clip phase boundaries for combat cancel rules. */
class CombatActionTimeline final {
public:
    void SetWindupEnd(const float normalizedTime) noexcept { windupEnd = normalizedTime; }
    void SetActiveEnd(const float normalizedTime) noexcept { activeEnd = normalizedTime; }

    [[nodiscard]] float GetWindupEnd() const noexcept { return windupEnd; }
    [[nodiscard]] float GetActiveEnd() const noexcept { return activeEnd; }

    [[nodiscard]] CombatPhaseKind ResolvePhase(const float normalizedTime) const noexcept;

private:
    float windupEnd = 0.22F;
    float activeEnd = 0.62F;
};

}  // namespace Spark
