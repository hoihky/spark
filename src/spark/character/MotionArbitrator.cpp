#include "spark/character/MotionArbitrator.hpp"

#include "spark/character/ICharacterMotor.hpp"

namespace Spark {

void MotionArbitrator::BeginFrame() noexcept {
    gameplayWish = Vector3::Zero;
    rootMotionAccumulated = Vector3::Zero;
}

void MotionArbitrator::AccumulateRootMotionDelta(const Vector3& worldDelta) noexcept {
    rootMotionAccumulated.x += worldDelta.x;
    rootMotionAccumulated.y += worldDelta.y;
    rootMotionAccumulated.z += worldDelta.z;
}

void MotionArbitrator::ApplyGameplayToMotor(ICharacterMotor& motor) const {
    switch (arbitrationMode) {
        case MotionArbitrationMode::RootMotionExclusive:
        case MotionArbitrationMode::InPlace:
            motor.SetGameplayMoveInput(Vector3::Zero);
            break;
        case MotionArbitrationMode::GameplayDriven:
        case MotionArbitrationMode::RootMotionPlusInput:
        default:
            motor.SetGameplayMoveInput(gameplayWish);
            break;
    }
}

void MotionArbitrator::ApplyRootMotionToMotor(ICharacterMotor& motor) const {
    switch (arbitrationMode) {
        case MotionArbitrationMode::RootMotionExclusive:
        case MotionArbitrationMode::RootMotionPlusInput:
            motor.ApplyWorldDelta(rootMotionAccumulated);
            break;
        case MotionArbitrationMode::GameplayDriven:
        case MotionArbitrationMode::InPlace:
        default:
            break;
    }
}

}  // namespace Spark
