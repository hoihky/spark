#include "spark/scene/water/WaterWaveSettings.hpp"

#include "spark/scene/water/GerstnerWave.hpp"

#include <algorithm>

namespace Spark {

void WaterWaveSettings::SetActiveWaveCount(const std::uint32_t count) noexcept {
    activeWaveCount = static_cast<std::uint32_t>(std::min<std::size_t>(count, kMaxWaves));
}

WaterWaveSettings WaterWaveSettings::FromPreset(const WaterWavePresetId preset, const float globalWindSpeedIn) noexcept {
    WaterWaveSettings settings{};
    settings.globalWindSpeed = globalWindSpeedIn;

    switch (preset) {
    case WaterWavePresetId::CalmLake:
        settings.waves[0] = GerstnerWave::FromDirection(1.0F, 0.15F, 0.045F, 9.0F, 0.55F, 0.35F);
        settings.waves[1] = GerstnerWave::FromDirection(-0.35F, 0.92F, 0.028F, 5.5F, 0.72F, 0.30F);
        settings.waves[2] = GerstnerWave::FromDirection(0.62F, -0.78F, 0.016F, 3.2F, 0.95F, 0.25F);
        settings.activeWaveCount = 3;
        break;
    case WaterWavePresetId::OceanModerate:
        settings.waves[0] = GerstnerWave::FromDirection(0.92F, 0.38F, 0.12F, 14.0F, 0.85F, 0.45F);
        settings.waves[1] = GerstnerWave::FromDirection(-0.48F, 0.88F, 0.075F, 8.0F, 1.05F, 0.40F);
        settings.waves[2] = GerstnerWave::FromDirection(0.22F, -0.97F, 0.04F, 4.5F, 1.25F, 0.35F);
        settings.waves[3] = GerstnerWave::FromDirection(0.71F, 0.71F, 0.025F, 2.8F, 1.45F, 0.28F);
        settings.activeWaveCount = 4;
        break;
    case WaterWavePresetId::StormySea:
        settings.waves[0] = GerstnerWave::FromDirection(0.78F, 0.62F, 0.22F, 18.0F, 1.35F, 0.55F);
        settings.waves[1] = GerstnerWave::FromDirection(-0.62F, 0.78F, 0.14F, 10.0F, 1.55F, 0.50F);
        settings.waves[2] = GerstnerWave::FromDirection(0.35F, -0.94F, 0.09F, 5.5F, 1.85F, 0.45F);
        settings.waves[3] = GerstnerWave::FromDirection(-0.88F, -0.47F, 0.06F, 3.5F, 2.05F, 0.40F);
        settings.activeWaveCount = 4;
        break;
    }

    if (globalWindSpeedIn != 1.0F && globalWindSpeedIn > 0.0F) {
        for (std::size_t i = 0; i < settings.activeWaveCount; ++i) {
            settings.waves[i].speed *= globalWindSpeedIn;
        }
    }
    return settings;
}

}  // namespace Spark
