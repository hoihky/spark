#include "spark/scene/foliage/WindFrameState.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/foliage/WindSettings.hpp"

#include <cmath>

namespace Spark {

void WindFrameState::ResetToCalm() noexcept {
    active = false;
    directionXZ = {1.0F, 0.0F, 0.0F};
    effectiveSpeed = 0.0F;
    gustAmplitude = 0.0F;
    gustFrequency = 0.0F;
    turbulence = 0.0F;
    simulationTimeSeconds = 0.0F;
}

void WindFrameState::SetFromSettings(const WindSettings& settings, const float timeSeconds) noexcept {
    simulationTimeSeconds = timeSeconds;
    if (!settings.IsEnabled() || settings.GetBaseSpeedMetersPerSecond() <= 0.0F) {
        ResetToCalm();
        return;
    }
    active = true;
    directionXZ = settings.NormalizedDirectionXZ();
    effectiveSpeed = settings.GetBaseSpeedMetersPerSecond();
    gustAmplitude = settings.GetGustAmplitude();
    gustFrequency = settings.GetGustFrequencyHertz();
    turbulence = settings.GetTurbulence();
}

void WindFrameState::WriteToSceneRenderParams(SceneRenderParams& params) const noexcept {
    params.windDirectionXZ = directionXZ;
    params.windEffectiveSpeed = active ? effectiveSpeed : 0.0F;
    params.windGustAmplitude = active ? gustAmplitude : 0.0F;
    params.windGustFrequency = active ? gustFrequency : 0.0F;
    params.windTurbulence = active ? turbulence : 0.0F;
    params.windSimulationTimeSeconds = simulationTimeSeconds;
    params.windActive = active;
}

}  // namespace Spark
