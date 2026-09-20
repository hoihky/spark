#ifndef SPARK_WATER_BACKGROUND_GLSL
#define SPARK_WATER_BACKGROUND_GLSL

#include "water_screen.glsl"

layout(set = 0, binding = 13) uniform sampler2D sceneOpaqueColor;
layout(set = 0, binding = 14) uniform sampler2D sceneOpaqueDepth;

const float WATER_AIR_IOR_RATIO = 1.0 / 1.333;
const float WATER_REFRACTION_PARALLAX_METERS = 0.22;
const float WATER_REFRACTION_PARALLAX_SCALE = 2.4;
const float WATER_REFRACTION_NORMAL_SCALE = 0.12;
/** Cleared far-plane depth (1.0) when sky does not write depth; also matches scene.vert sky inset. */
const float WATER_SCENE_DEPTH_SKY = 0.9999;
/** Reflection / SSR samples binding 13. */
const float WATER_OPAQUE_HDR_EXPOSURE = 0.62;
/** Refraction path — lower exposure so head-on views do not blow out to pale HDR sky. */
const float WATER_REFRACT_HDR_EXPOSURE = 0.38;
/** Fixed column depth for open-ocean depth anchor (subtle, not flat fill). */
const float WATER_OPEN_OCEAN_COLUMN_DEPTH = 3.0;

vec3 waterScaleOpaqueHdr(vec3 hdrRgb, float exposure) {
    vec3 x = max(hdrRgb, vec3(0.0)) * exposure;
    return x / (vec3(1.0) + x);
}

vec3 waterScaleOpaqueHdr(vec3 hdrRgb) {
    return waterScaleOpaqueHdr(hdrRgb, WATER_OPAQUE_HDR_EXPOSURE);
}

vec2 waterComputeRefractScreenUv(
        vec2 uvDirect,
        vec3 worldPos,
        vec3 N,
        vec3 V,
        float roughness) {
    vec3 safeN = normalize(N);
    vec3 safeV = normalize(V);

    vec3 refractDir = refract(-safeV, safeN, WATER_AIR_IOR_RATIO);
    if (dot(refractDir, refractDir) < 1e-6) {
        refractDir = reflect(-safeV, safeN);
    } else {
        refractDir = normalize(refractDir);
    }

    vec2 uvParallax = waterWorldToScreenUv(worldPos + refractDir * WATER_REFRACTION_PARALLAX_METERS);
    vec2 parallaxOffset = (uvParallax - uvDirect) * WATER_REFRACTION_PARALLAX_SCALE;

    float ripple = 1.0 - clamp(roughness, 0.02, 1.0);
    vec2 normalOffset = safeN.xz * WATER_REFRACTION_NORMAL_SCALE * (0.35 + 0.65 * ripple);

    float grazing = 1.0 - max(dot(safeN, safeV), 0.0);
    float offsetWeight = mix(0.62, 0.28, pow(grazing, 1.2));
    return clamp(uvDirect + parallaxOffset * offsetWeight + normalOffset, vec2(0.001), vec2(0.999));
}

vec3 waterSampleRefractedOpaqueAtUv(vec2 screenUv) {
    return waterScaleOpaqueHdr(texelFetch(sceneOpaqueColor, waterOpaquePixel(screenUv), 0).rgb);
}

/** Underwater refraction only — dimmer than reflection so low-fresnel body stays turquoise. */
vec3 waterSampleRefractOpaqueAtUv(vec2 screenUv) {
    return waterScaleOpaqueHdr(
            texelFetch(sceneOpaqueColor, waterOpaquePixel(screenUv), 0).rgb, WATER_REFRACT_HDR_EXPOSURE);
}

float waterSampleSceneDepthAtUv(vec2 screenUv) {
    return texelFetch(sceneOpaqueDepth, waterOpaquePixel(screenUv), 0).r;
}

bool waterSceneDepthIsSky(float sceneDepth) {
    return sceneDepth >= WATER_SCENE_DEPTH_SKY;
}

float waterComputeColumnDepth(vec2 screenUv, float waterSurfaceY) {
    float sceneDepth = waterSampleSceneDepthAtUv(screenUv);
    if (sceneDepth >= WATER_SCENE_DEPTH_SKY) {
        return 0.0;
    }
    vec3 sceneWorld = waterReconstructWorld(screenUv, sceneDepth);
    return max(waterSurfaceY - sceneWorld.y, 0.0);
}

vec3 waterApplyDepthAbsorption(vec3 shallowRgb, vec3 deepRgb, float columnDepth, float absorption) {
    float t = 1.0 - exp(-max(absorption, 0.0) * columnDepth);
    return mix(shallowRgb, deepRgb, clamp(t, 0.0, 1.0));
}

/** View-independent open-ocean surface color (same tint everywhere on the lake). */
vec3 waterOpenOceanTint(vec3 shallowRgb, vec3 deepRgb, float absorption) {
    return waterApplyDepthAbsorption(shallowRgb, deepRgb, WATER_OPEN_OCEAN_COLUMN_DEPTH, absorption);
}

/** Reflection: light tint — preserve HDR sky brightness variation and ripple contrast. */
vec3 waterGradeReflectionHdr(vec3 hdr, vec3 shallowRgb, vec3 deepRgb) {
    vec3 tinted = hdr * mix(vec3(1.0), shallowRgb * 1.18 + deepRgb * 0.22, 0.40);
    return tinted / (vec3(1.0) + tinted * 0.22);
}

/** Refraction: stronger water-color anchor so head-on views stay turquoise, not white. */
vec3 waterGradeRefractHdr(vec3 hdr, vec3 shallowRgb, vec3 deepRgb) {
    vec3 tinted = hdr * mix(vec3(1.0), shallowRgb * 1.32 + deepRgb * 0.48, 0.58);
    tinted = tinted / (vec3(1.0) + tinted * 0.42);
    vec3 anchor = mix(shallowRgb, deepRgb, 0.55);
    return mix(tinted, anchor, 0.28);
}

/** Schlick fresnel with a modest floor — real view variation without pale blowout. */
float waterComputeBodyFresnel(float macroNdotV) {
    float ndv = clamp(macroNdotV, 0.001, 1.0);
    float F = 0.02 + 0.98 * pow(1.0 - ndv, 5.0);
    return clamp(F + 0.14 * pow(1.0 - ndv, 1.6), 0.26, 0.93);
}

/** Shallow shoreline band only (column between surface and seabed/terrain). */
float waterShoreBlendWeight(float columnDepth) {
    const float near = 0.05;
    const float far = 0.62;
    return smoothstep(far, near, columnDepth) * smoothstep(0.0, near, columnDepth + 0.001);
}

#endif
