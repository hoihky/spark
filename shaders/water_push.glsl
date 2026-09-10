#ifndef SPARK_WATER_PUSH_GLSL
#define SPARK_WATER_PUSH_GLSL

#include "gerstner_wave.glsl"

layout(push_constant) uniform WaterPush {
    mat4 model;
    vec4 baseColor;
    float timeSeconds;
    int waveCount;
    int shadowFlags;
    float roughness;
    float padding0;
    float padding1;  // std430: align waves[4] to offset 104 (matches C++ WaterPushConstants)
    WaterGerstnerWaveGpu waves[4];
} waterPush;

#endif
