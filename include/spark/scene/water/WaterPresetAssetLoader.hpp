#pragma once

#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/scene/water/WaterPresetAsset.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"

namespace Spark {

/** Loads <c>.sparkwater</c> wave preset files from the assets tree. */
class WaterPresetAssetLoader final {
public:
    [[nodiscard]] static AssetLoadOutcome<WaterPresetAsset> TryLoadFromFile(const char* path);

    [[nodiscard]] static AssetLoadOutcome<WaterPresetAsset> TryLoadPreset(WaterWavePresetId presetId) noexcept;

    [[nodiscard]] static const char* PresetFileName(WaterWavePresetId presetId) noexcept;

    [[nodiscard]] static Utf8String ResolveReadablePath(const char* keyOrPath);
};

}  // namespace Spark
