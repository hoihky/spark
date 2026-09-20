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
    float wavesStd430Padding[2];
    vec4 deepColor;
    float foamStrength;
    float detailNormalStrength;
    float shorelineFoamStrength;
    float shorelineFoamMaxDepth;
    vec2 tileAnchorXZ;
    float ssrEnabled;
    float ssrMaxRayDistance;
    float ssrThickness;
    float ssrStepScale;
    int ssrMaxSteps;
    float ssrStrength;
    float ssrHorizonFadeStart;
    float ssrHorizonFadeEnd;
    float ssrHalfRes;
} waterPush;

#endif
