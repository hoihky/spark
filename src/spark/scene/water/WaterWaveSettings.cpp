#include "spark/scene/water/WaterWaveSettings.hpp"

#include "spark/scene/water/WaterPresetAssetLoader.hpp"

#include <algorithm>

namespace Spark {

void WaterWaveSettings::SetActiveWaveCount(const std::uint32_t count) noexcept {
    activeWaveCount = static_cast<std::uint32_t>(std::min<std::size_t>(count, kMaxWaves));
}

WaterWaveSettings WaterWaveSettings::FromPreset(const WaterWavePresetId preset, const float globalWindSpeedIn) noexcept {
    const AssetLoadOutcome<WaterPresetAsset> loaded = WaterPresetAssetLoader::TryLoadPreset(preset);
    WaterWaveSettings settings = loaded.value.settings;
    if (globalWindSpeedIn > 0.0F && globalWindSpeedIn != settings.GetGlobalWindSpeed()) {
        const float ratio = globalWindSpeedIn / settings.GetGlobalWindSpeed();
        for (std::size_t i = 0; i < settings.GetActiveWaveCount(); ++i) {
            settings.GetWave(i).speed *= ratio;
        }
        settings.SetGlobalWindSpeed(globalWindSpeedIn);
    }
    return settings;
}

}  // namespace Spark
