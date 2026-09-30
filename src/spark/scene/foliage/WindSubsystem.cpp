#include "spark/scene/foliage/WindSubsystem.hpp"

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

void WindSubsystem::Reset() noexcept {
    activeSettings = WindSettings{};
    hasEnvironment = false;
    simulationTimeSeconds = 0.0F;
    frameState.ResetToCalm();
}

void WindSubsystem::ApplyEnvironmentSettings(const WindSettings& settings) noexcept {
    activeSettings = settings;
    hasEnvironment = true;
}

void WindSubsystem::AdvanceSimulation(const float deltaTimeSeconds) noexcept {
    if (deltaTimeSeconds > 0.0F) {
        simulationTimeSeconds += deltaTimeSeconds;
    }
    if (!hasEnvironment) {
        frameState.ResetToCalm();
        return;
    }
    frameState.SetFromSettings(activeSettings, simulationTimeSeconds);
}

void WindSubsystem::WriteFrameToSceneRenderParams(SceneRenderParams& params) const noexcept {
    if (!hasEnvironment) {
        WindFrameState calm{};
        calm.ResetToCalm();
        calm.WriteToSceneRenderParams(params);
        return;
    }
    frameState.WriteToSceneRenderParams(params);
}

}  // namespace Spark
