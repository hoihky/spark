#pragma once

#include "spark/scene/water/GerstnerWave.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Up to four Gerstner waves shared by CPU gameplay math and the water shader. */
class WaterWaveSettings {
public:
    static constexpr std::size_t kMaxWaves = 4;

    [[nodiscard]] static WaterWaveSettings FromPreset(WaterWavePresetId preset, float globalWindSpeed = 1.0F) noexcept;

    [[nodiscard]] std::uint32_t GetActiveWaveCount() const noexcept { return activeWaveCount; }
    void SetActiveWaveCount(std::uint32_t count) noexcept;

    [[nodiscard]] const GerstnerWave& GetWave(std::size_t index) const noexcept { return waves[index]; }
    [[nodiscard]] GerstnerWave& GetWave(std::size_t index) noexcept { return waves[index]; }

    [[nodiscard]] float GetGlobalWindSpeed() const noexcept { return globalWindSpeed; }
    void SetGlobalWindSpeed(float value) noexcept { globalWindSpeed = value; }

    [[nodiscard]] const GerstnerWave* GetWavesData() const noexcept { return waves; }

private:
    GerstnerWave waves[kMaxWaves]{};
    std::uint32_t activeWaveCount = 0;
    float globalWindSpeed = 1.0F;
};

}  // namespace Spark
