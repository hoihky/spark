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
    float waterLevelY;
    float absorption;
    WaterGerstnerWaveGpu waves[4];
    vec4 deepColor;
} waterPush;

#endif
