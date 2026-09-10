#pragma once

#include <cstdint>

namespace Spark {

/** Named Gerstner/wave parameter set (shader/simulation preset; resolved in later milestones). */
enum class WaterWavePresetId : std::uint8_t {
    CalmLake = 0,
    OceanModerate = 1,
    StormySea = 2,
};

}  // namespace Spark
