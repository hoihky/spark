#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/** GPU layout for <c>shaders/water_push.glsl</c> (std430 push constants). */
struct WaterGerstnerWaveGpu {
    float directionX = 0.0F;
    float directionZ = 0.0F;
    float amplitude = 0.0F;
    float wavelength = 1.0F;
    float speed = 0.0F;
    float steepness = 0.0F;
    float padding[2]{};
};

struct WaterPushConstants {
    float model[16]{};
    float baseColor[4]{0.05F, 0.30F, 0.50F, 1.0F};
    float timeSeconds = 0.0F;
    std::int32_t waveCount = 0;
    std::int32_t shadowFlags = 0;
    float roughness = 0.04F;
    float waterLevelY = 0.0F;
    float absorption = 0.35F;
    WaterGerstnerWaveGpu waves[4]{};
    float deepColor[4]{0.02F, 0.12F, 0.28F, 1.0F};
    /** std430 rounds the push-constant block to 256 bytes (SPIR-V range [0, 256]). */
    float blockPadding[2]{};
};

static_assert(sizeof(WaterGerstnerWaveGpu) == 32);
static_assert(sizeof(WaterPushConstants) == 256);
static_assert(offsetof(WaterPushConstants, waves) == 104);
static_assert(offsetof(WaterPushConstants, deepColor) == 232);

}  // namespace Spark
