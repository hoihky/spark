#pragma once

#include "spark/scene/water/GerstnerWaveSurface.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

namespace Spark {

/** Named wave configuration used by demos, ECS, and gameplay sampling. */
class WaterWavePreset {
public:
    explicit WaterWavePreset(WaterWavePresetId id = WaterWavePresetId::CalmLake) noexcept;

    [[nodiscard]] WaterWavePresetId GetId() const noexcept { return id; }
    [[nodiscard]] const char* GetLabel() const noexcept;

    [[nodiscard]] WaterWaveSettings ToSettings(float globalWindSpeed = 1.0F) const noexcept;
    [[nodiscard]] GerstnerWaveSurface ToSurface(float globalWindSpeed = 1.0F) const noexcept;

    [[nodiscard]] static WaterWavePreset Next(WaterWavePresetId current) noexcept;

private:
    WaterWavePresetId id = WaterWavePresetId::CalmLake;
};

}  // namespace Spark
