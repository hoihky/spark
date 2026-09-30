#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

struct SceneRenderParams;

/** Per-frame wind values uploaded with the scene UBO (F0). */
class WindFrameState {
public:
    void ResetToCalm() noexcept;

    void SetFromSettings(const class WindSettings& settings, float simulationTimeSeconds) noexcept;

    [[nodiscard]] bool IsActive() const noexcept { return active; }
    [[nodiscard]] Vector3 GetDirectionXZ() const noexcept { return directionXZ; }
    [[nodiscard]] float GetEffectiveSpeed() const noexcept { return effectiveSpeed; }
    [[nodiscard]] float GetGustAmplitude() const noexcept { return gustAmplitude; }
    [[nodiscard]] float GetGustFrequency() const noexcept { return gustFrequency; }
    [[nodiscard]] float GetTurbulence() const noexcept { return turbulence; }
    [[nodiscard]] float GetSimulationTimeSeconds() const noexcept { return simulationTimeSeconds; }

    void WriteToSceneRenderParams(SceneRenderParams& params) const noexcept;

private:
    bool active = false;
    Vector3 directionXZ{1.0F, 0.0F, 0.0F};
    float effectiveSpeed = 0.0F;
    float gustAmplitude = 0.0F;
    float gustFrequency = 0.0F;
    float turbulence = 0.0F;
    float simulationTimeSeconds = 0.0F;
};

}  // namespace Spark
