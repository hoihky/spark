#ifndef SPARK_WATER_FOAM_GLSL
#define SPARK_WATER_FOAM_GLSL

#include "gerstner_wave.glsl"

const float WATER_FOAM_CREST_JACOBIAN_START = 0.88;
const float WATER_FOAM_CREST_JACOBIAN_END = 0.52;

/** White foam on steep Gerstner crests (Jacobian threshold + world-space noise). */
float waterCrestFoamMask(vec2 worldXZ, float timeSeconds, int waveCount, WaterGerstnerWaveGpu waves[4], float strength) {
    float jacobian = sparkGerstnerJacobian(worldXZ, timeSeconds, waveCount, waves);
    float crest = smoothstep(WATER_FOAM_CREST_JACOBIAN_END, WATER_FOAM_CREST_JACOBIAN_START, jacobian);
    float noise = 0.5 + 0.5 * sin(dot(worldXZ * 0.13, vec2(5.17, 3.91)) + timeSeconds * 2.35);
    crest *= smoothstep(0.40, 0.80, noise + crest * 0.38);
    return clamp(crest * strength, 0.0, 1.0);
}

#endif
