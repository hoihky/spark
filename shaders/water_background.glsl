#ifndef SPARK_WATER_BACKGROUND_GLSL
#define SPARK_WATER_BACKGROUND_GLSL

#include "scene_screen.glsl"

layout(set = 0, binding = 13) uniform sampler2D sceneOpaqueColor;
layout(set = 0, binding = 14) uniform sampler2D sceneOpaqueDepth;

const float WATER_AIR_IOR_RATIO = 1.0 / 1.333;
const float WATER_REFRACTION_PARALLAX_METERS = 0.22;
const float WATER_REFRACTION_PARALLAX_SCALE = 2.4;
const float WATER_REFRACTION_NORMAL_SCALE = 0.052;
const float WATER_SCENE_DEPTH_SKY = 0.9999;

vec2 waterComputeRefractScreenUv(vec3 worldPos, vec3 N, vec3 V, float roughness) {
    vec3 safeN = normalize(N);
    vec3 safeV = normalize(V);

    vec3 refractDir = refract(-safeV, safeN, WATER_AIR_IOR_RATIO);
    if (dot(refractDir, refractDir) < 1e-6) {
        refractDir = reflect(-safeV, safeN);
    } else {
        refractDir = normalize(refractDir);
    }

    vec2 uvDirect = sparkSceneScreenUvFromWorld(worldPos);
    vec2 uvParallax = sparkSceneScreenUvFromWorld(worldPos + refractDir * WATER_REFRACTION_PARALLAX_METERS);
    vec2 parallaxOffset = (uvParallax - uvDirect) * WATER_REFRACTION_PARALLAX_SCALE;

    float ripple = 1.0 - clamp(roughness, 0.02, 1.0);
    vec2 normalOffset = safeN.xz * WATER_REFRACTION_NORMAL_SCALE * (0.30 + 0.70 * ripple);

    float grazing = 1.0 - max(dot(safeN, safeV), 0.0);
    float offsetWeight = 0.55 + 0.45 * grazing;
    return clamp(uvDirect + parallaxOffset * offsetWeight + normalOffset, vec2(0.001), vec2(0.999));
}

vec3 waterSampleRefractedOpaqueAtUv(vec2 screenUv) {
    return texture(sceneOpaqueColor, screenUv).rgb;
}

/** Screen-space refraction of the opaque HDR background (binding 13). */
vec3 waterSampleRefractedOpaque(vec3 worldPos, vec3 N, vec3 V, float roughness) {
    return waterSampleRefractedOpaqueAtUv(waterComputeRefractScreenUv(worldPos, N, V, roughness));
}

float waterSampleSceneDepthAtUv(vec2 screenUv) {
    return texture(sceneOpaqueDepth, screenUv).r;
}

/** Vertical water column from the displaced surface to reconstructed scene geometry (meters). */
float waterComputeColumnDepth(vec2 screenUv, float waterSurfaceY) {
    float sceneDepth = waterSampleSceneDepthAtUv(screenUv);
    if (sceneDepth >= WATER_SCENE_DEPTH_SKY) {
        return 0.0;
    }
    vec3 sceneWorld = sparkSceneReconstructWorld(screenUv, sceneDepth);
    return max(waterSurfaceY - sceneWorld.y, 0.0);
}

/** Beer–Lambert shallow → deep tint (artist colors). */
vec3 waterApplyDepthAbsorption(vec3 shallowRgb, vec3 deepRgb, float columnDepth, float absorption) {
    float t = 1.0 - exp(-max(absorption, 0.0) * columnDepth);
    return mix(shallowRgb, deepRgb, clamp(t, 0.0, 1.0));
}

#endif
