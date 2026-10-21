#include "spark/physics/simulation/PhysicsFixedStepClock.hpp"

#include <algorithm>

namespace Spark {

void PhysicsFixedStepClock::SetFixedTimestepSeconds(const float seconds) noexcept {
    fixedTimestepSeconds = (seconds > 0.0F) ? seconds : 0.0F;
    if (fixedTimestepSeconds <= 0.0F) {
        accumulatorSeconds = 0.0F;
    }
}

void PhysicsFixedStepClock::SetMaxStepsPerFrame(const int maxSteps) noexcept {
    maxStepsPerFrame = std::max(1, maxSteps);
}

void PhysicsFixedStepClock::ResetAccumulator() noexcept {
    accumulatorSeconds = 0.0F;
}

int PhysicsFixedStepClock::BeginFrame(const float frameDeltaSeconds) noexcept {
    if (frameDeltaSeconds <= 0.0F) {
        stepDeltaSeconds = 0.0F;
        return 0;
    }

    if (fixedTimestepSeconds <= 0.0F) {
        stepDeltaSeconds = frameDeltaSeconds;
        return 1;
    }

    accumulatorSeconds += frameDeltaSeconds;
    const float maxAccumulated = fixedTimestepSeconds * static_cast<float>(maxStepsPerFrame);
    accumulatorSeconds = std::min(accumulatorSeconds, maxAccumulated);

    int steps = 0;
    while (accumulatorSeconds >= fixedTimestepSeconds && steps < maxStepsPerFrame) {
        accumulatorSeconds -= fixedTimestepSeconds;
        ++steps;
    }

    stepDeltaSeconds = fixedTimestepSeconds;
    return steps;
}

}  // namespace Spark
