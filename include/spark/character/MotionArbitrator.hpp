#pragma once

#include "spark/character/MotionArbitrationMode.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class ICharacterMotor;

/** Combines gameplay wish velocity and animation root-motion deltas per arbitration mode. */
class MotionArbitrator {
public:
    void SetMode(const MotionArbitrationMode mode) noexcept { arbitrationMode = mode; }
    [[nodiscard]] MotionArbitrationMode GetMode() const noexcept { return arbitrationMode; }

    void BeginFrame() noexcept;
    void SetGameplayWishVelocity(const Vector3& worldVelocity) noexcept { gameplayWish = worldVelocity; }
    void AccumulateRootMotionDelta(const Vector3& worldDelta) noexcept;

    /** Writes move input on <c>motor</c> from gameplay wish (call before controller simulation). */
    void ApplyGameplayToMotor(ICharacterMotor& motor) const;

    /** Applies accumulated root motion after animation tick (call after root motion sampling). */
    void ApplyRootMotionToMotor(ICharacterMotor& motor) const;

private:
    MotionArbitrationMode arbitrationMode = MotionArbitrationMode::GameplayDriven;
    Vector3 gameplayWish{Vector3::Zero};
    Vector3 rootMotionAccumulated{Vector3::Zero};
};

}  // namespace Spark
