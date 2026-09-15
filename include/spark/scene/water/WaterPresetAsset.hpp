#pragma once

#include "spark/scene/water/WaterWavePresetId.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

namespace Spark {

/** Disk-backed Gerstner preset (see <c>assets/water/</c> and <c>.sparkwater</c> files). */
struct WaterPresetAsset {
    WaterWavePresetId presetId = WaterWavePresetId::CalmLake;
    WaterWaveSettings settings{};
};

}  // namespace Spark
