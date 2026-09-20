#ifndef SPARK_WATER_FOAM_GLSL
#define SPARK_WATER_FOAM_GLSL

#include "gerstner_wave.glsl"

/** Jacobian near 1.0 = no compression; lower values = steep / folding crests. */
const float WATER_FOAM_CREST_JACOBIAN_LO = 0.58;
const float WATER_FOAM_CREST_JACOBIAN_HI = 0.90;

/**
 * White foam on breaking Gerstner crests.
 * Uses macro (Gerstner) slope only — detail normals and hard NdotV gates made foam vanish
 * or sparkle at most camera angles.
 */
float waterCrestFoamMask(
        vec2 worldXZ,
        float timeSeconds,
        int waveCount,
        WaterGerstnerWaveGpu waves[4],
        float strength,
        vec3 macroNormal,
        vec3 viewDir) {
    float jacobian = sparkGerstnerJacobian(worldXZ, timeSeconds, waveCount, waves);
    // Foam when the surface compresses (J drops), not on broad gentle wave backs (J ~ 1).
    float breaking = 1.0 - smoothstep(WATER_FOAM_CREST_JACOBIAN_LO, WATER_FOAM_CREST_JACOBIAN_HI, jacobian);

    float steep = 1.0 - clamp(macroNormal.y, 0.0, 1.0);
    breaking *= smoothstep(0.02, 0.16, steep);

    float noise = 0.5 + 0.5 * sin(dot(worldXZ * 0.13, vec2(5.17, 3.91)) + timeSeconds * 2.35);
    breaking *= smoothstep(0.44, 0.80, noise + breaking * 0.30);
    breaking = pow(clamp(breaking, 0.0, 1.0), 1.5);

    vec3 V = normalize(viewDir);
    float macroNdotV = max(dot(macroNormal, V), 0.0);
    // Gentle view bias from macro slope only; never fully cull foam by camera pitch.
    float viewBias = mix(0.78, 1.0, pow(clamp(1.0 - macroNdotV, 0.0, 1.0), 0.4));

    return clamp(breaking * viewBias * strength, 0.0, 1.0);
}

/**
 * Shoreline foam from scene depth (binding 14): shallow column between water surface and seabed/terrain.
 * GPU approximation of terrain height — matches W2-06 depth-reconstruction path.
 */
float waterShorelineFoamMask(
        vec2 screenUv,
        float waterSurfaceY,
        float maxDepth,
        float strength,
        float timeSeconds,
        vec2 worldXZ) {
    float sceneDepth = waterSampleSceneDepthAtUv(screenUv);
    if (sceneDepth >= WATER_SCENE_DEPTH_SKY) {
        return 0.0;
    }
    float shoreColumn = waterComputeColumnDepth(screenUv, waterSurfaceY);
    if (shoreColumn <= 0.0) {
        return 0.0;
    }
    float maxBand = max(maxDepth, 0.05);
    float band = smoothstep(maxBand, 0.0, shoreColumn);
    float edge = smoothstep(0.0, 0.10, shoreColumn);
    float noise = 0.5 + 0.5 * sin(dot(worldXZ * 0.19, vec2(4.3, 6.1)) + timeSeconds * 1.75);
    band *= smoothstep(0.32, 0.78, noise + band * 0.42);
    return clamp(band * edge * strength, 0.0, 1.0);
}

#endif
