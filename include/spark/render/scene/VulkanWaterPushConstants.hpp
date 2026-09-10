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
    float padding0 = 0.0F;
    float padding1 = 0.0F;  // std430: align waves[4] to 8 bytes (SPIR-V offset 104)
    WaterGerstnerWaveGpu waves[4]{};
};

static_assert(sizeof(WaterGerstnerWaveGpu) == 32);
static_assert(sizeof(WaterPushConstants) == 232);
static_assert(offsetof(WaterPushConstants, waves) == 104);

}  // namespace Spark
