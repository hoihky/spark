#pragma once

namespace Spark {

/**
 * Accumulates real frame time and emits a bounded number of fixed simulation steps per frame.
 * When fixed timestep is disabled (<c>fixedTimestepSeconds <= 0</c>), <c>BeginFrame</c> reports one step
 * equal to the frame delta.
 */
class PhysicsFixedStepClock {
public:
    void SetFixedTimestepSeconds(float seconds) noexcept;
    void SetMaxStepsPerFrame(int maxSteps) noexcept;
    void ResetAccumulator() noexcept;

    [[nodiscard]] float GetFixedTimestepSeconds() const noexcept { return fixedTimestepSeconds; }
    [[nodiscard]] int GetMaxStepsPerFrame() const noexcept { return maxStepsPerFrame; }
    [[nodiscard]] float GetAccumulatorSeconds() const noexcept { return accumulatorSeconds; }

    /**
     * @return Number of simulation steps to run this frame (each step uses <c>GetStepDeltaSeconds</c>).
     */
    [[nodiscard]] int BeginFrame(float frameDeltaSeconds) noexcept;

    /** Delta time for one simulation step after the latest <c>BeginFrame</c> call. */
    [[nodiscard]] float GetStepDeltaSeconds() const noexcept { return stepDeltaSeconds; }

private:
    float fixedTimestepSeconds = 0.0F;
    float accumulatorSeconds = 0.0F;
    float stepDeltaSeconds = 0.0F;
    int maxStepsPerFrame = 8;
};

}  // namespace Spark
