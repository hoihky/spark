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
    /** std430 aligns the following vec4 to a 16-byte boundary (SPIR-V offset 240). */
    float wavesStd430Padding[2]{};
    float deepColor[4]{0.02F, 0.12F, 0.28F, 1.0F};
    float foamStrength = 0.85F;
    float detailNormalStrength = 0.34F;
    float shorelineFoamStrength = 0.90F;
    float shorelineFoamMaxDepth = 2.0F;
    /** Infinite-ocean clipmap anchor (XZ); added to mesh local position for world-stable Gerstner. */
    float tileAnchorXZ[2]{};
    /** Screen-space reflection (W3-02); 0 disables SSR and preserves legacy sky reflection. */
    float ssrEnabled = 1.0F;
    float ssrMaxRayDistance = 48.0F;
    float ssrThickness = 0.15F;
    float ssrStepScale = 1.0F;
    std::int32_t ssrMaxSteps = 24;
    float ssrStrength = 1.0F;
    float ssrHorizonFadeStart = -0.02F;
    float ssrHorizonFadeEnd = -0.30F;
    /** When 1, SSR ray march quantizes UV to a 2×2 pixel grid (half-res sampling). */
    float ssrHalfRes = 0.0F;
};

static_assert(sizeof(WaterGerstnerWaveGpu) == 32);
static_assert(sizeof(WaterPushConstants) == 316);
static_assert(offsetof(WaterPushConstants, waves) == 104);
static_assert(offsetof(WaterPushConstants, wavesStd430Padding) == 232);
static_assert(offsetof(WaterPushConstants, deepColor) == 240);
static_assert(offsetof(WaterPushConstants, foamStrength) == 256);
static_assert(offsetof(WaterPushConstants, detailNormalStrength) == 260);
static_assert(offsetof(WaterPushConstants, shorelineFoamStrength) == 264);
static_assert(offsetof(WaterPushConstants, shorelineFoamMaxDepth) == 268);
static_assert(offsetof(WaterPushConstants, tileAnchorXZ) == 272);
static_assert(offsetof(WaterPushConstants, ssrEnabled) == 280);
static_assert(offsetof(WaterPushConstants, ssrMaxSteps) == 296);
static_assert(offsetof(WaterPushConstants, ssrHorizonFadeEnd) == 308);
static_assert(offsetof(WaterPushConstants, ssrHalfRes) == 312);

}  // namespace Spark
