#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

namespace Spark {

/** Rotates Gerstner wave travel axes (shared by presets and gameplay). */
class WaterWaveDirection {
public:
    /**
     * Rotates every active wave so wave 0's travel direction matches @p travelDirectionWorldXZ.
     * @p travelDirectionWorldXZ is the direction crests move on the XZ plane (not "from" wind).
     */
    static void RotateSettingsToPrimarySwell(WaterWaveSettings& settings, Vector2 travelDirectionWorldXZ) noexcept;
};

}  // namespace Spark
