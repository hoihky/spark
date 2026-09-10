#include "spark/scene/water/WaterWavePreset.hpp"

namespace Spark {

WaterWavePreset::WaterWavePreset(const WaterWavePresetId idIn) noexcept : id(idIn) {}

const char* WaterWavePreset::GetLabel() const noexcept {
    switch (id) {
    case WaterWavePresetId::CalmLake:
        return "CalmLake";
    case WaterWavePresetId::OceanModerate:
        return "OceanModerate";
    case WaterWavePresetId::StormySea:
        return "StormySea";
    }
    return "Water";
}

WaterWaveSettings WaterWavePreset::ToSettings(const float globalWindSpeed) const noexcept {
    return WaterWaveSettings::FromPreset(id, globalWindSpeed);
}

GerstnerWaveSurface WaterWavePreset::ToSurface(const float globalWindSpeed) const noexcept {
    return GerstnerWaveSurface::FromPreset(id, globalWindSpeed);
}

WaterWavePreset WaterWavePreset::Next(const WaterWavePresetId current) noexcept {
    switch (current) {
    case WaterWavePresetId::CalmLake:
        return WaterWavePreset(WaterWavePresetId::OceanModerate);
    case WaterWavePresetId::OceanModerate:
        return WaterWavePreset(WaterWavePresetId::StormySea);
    default:
        return WaterWavePreset(WaterWavePresetId::CalmLake);
    }
}

}  // namespace Spark
