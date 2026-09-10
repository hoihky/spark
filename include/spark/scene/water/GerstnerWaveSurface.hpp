#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

namespace Spark {

/**
 * CPU Gerstner heightfield evaluator for a fixed wave configuration.
 * Matches <c>shaders/gerstner_wave.glsl</c>.
 */
class GerstnerWaveSurface {
public:
    explicit GerstnerWaveSurface(WaterWaveSettings settingsIn) noexcept;

    [[nodiscard]] static GerstnerWaveSurface FromPreset(
            WaterWavePresetId preset,
            float globalWindSpeed = 1.0F) noexcept;

    [[nodiscard]] const WaterWaveSettings& GetSettings() const noexcept { return settings; }

    [[nodiscard]] float SampleHeight(float worldX, float worldZ, float timeSeconds) const noexcept;
    [[nodiscard]] Vector3 SampleNormal(float worldX, float worldZ, float timeSeconds) const noexcept;
    [[nodiscard]] Vector2 SampleHorizontalDisplacement(
            float worldX,
            float worldZ,
            float timeSeconds) const noexcept;

private:
    struct Accumulation {
        float height = 0.0F;
        Vector3 tangent{1.0F, 0.0F, 0.0F};
        Vector3 binormal{0.0F, 0.0F, 1.0F};
        Vector2 horizontalDisp{};
    };

    void AccumulateWave(
            const GerstnerWave& wave,
            float worldX,
            float worldZ,
            float timeSeconds,
            Accumulation& out) const noexcept;

    [[nodiscard]] Accumulation Evaluate(float worldX, float worldZ, float timeSeconds) const noexcept;

    WaterWaveSettings settings{};
};

}  // namespace Spark
