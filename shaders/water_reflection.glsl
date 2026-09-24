#ifndef SPARK_WATER_REFLECTION_GLSL
#define SPARK_WATER_REFLECTION_GLSL

#include "ibl.glsl"

/** Water uses a sharper env LOD than dielectric PBR (no 6.0 floor meant for walls/ground). */
float sparkWaterSpecularEnvLod(float roughness) {
    float r = clamp(roughness, 0.02, 1.0);
    return clamp(r * r * SPARK_HDR_ENV_MAX_LOD, 0.5, SPARK_HDR_ENV_MAX_LOD);
}

/** Legacy W1 sky reflection (diffuse + specular mix) used when SSR is disabled. */
vec3 waterLegacySkyReflection(vec3 N, vec3 V, float roughness) {
    int iblLayer = int(round(ubo.iblParams.x));
    bool hdrEnv = ubo.iblParams.z > 0.5;
    float iblIntensity = ubo.iblParams.y;
    vec2 envUvScale = sparkIblEnvLayerUvScale();
    vec3 R = reflect(-normalize(V), normalize(N));
    float specLod = sparkWaterSpecularEnvLod(roughness);
    vec3 skySpecular = sparkSampleEnvironment(
            sceneTextures, sceneHdrTextures, R, iblLayer, hdrEnv, envUvScale, specLod);
    vec3 skyDiffuse = sparkSampleEnvironment(
            sceneTextures,
            sceneHdrTextures,
            N,
            iblLayer,
            hdrEnv,
            envUvScale,
            SPARK_HDR_DIFFUSE_IBL_LOD);
    return mix(skyDiffuse, skySpecular, 0.68) * iblIntensity;
}

/**
 * Sample binding 13 along a surface normal. Reject non-sky depth so terrain/cubes
 * do not tint reflections differently per screen region.
 */
vec3 waterEvalReflectionFallback(
        vec2 surfaceUv,
        vec3 worldPos,
        vec3 surfaceN,
        vec3 V,
        vec3 shallowColor,
        vec3 deepColor) {
    vec3 safeV = normalize(V);
    vec3 safeN = normalize(surfaceN);
    vec3 R = reflect(-safeV, safeN);
    vec3 cam = ubo.cameraPos.xyz;

    vec2 sampleUv = surfaceUv;
    if (R.y > 0.02) {
        sampleUv = clamp(waterWorldToScreenUv(cam + R * 480.0), vec2(0.001), vec2(0.999));
    } else {
        const float probeDist = 0.65;
        vec2 uvR = waterWorldToScreenUv(worldPos + R * probeDist);
        vec2 uvV = waterWorldToScreenUv(worldPos - safeV * probeDist);
        sampleUv = clamp(surfaceUv + (uvR - uvV), vec2(0.001), vec2(0.999));
    }

    // Extra screen jitter from surface slope so detail ripples distort reflections.
    sampleUv += safeN.xz * 0.046;
    sampleUv = clamp(sampleUv, vec2(0.001), vec2(0.999));

    vec3 sampleRgb = waterSampleRefractedOpaqueAtUv(sampleUv);
    float sampleDepth = waterSampleSceneDepthAtUv(sampleUv);
    if (!waterSceneDepthIsSky(sampleDepth)) {
        sampleRgb = mix(sampleRgb, shallowColor * 0.42 + deepColor * 0.88, 0.88);
    }
    return waterGradeReflectionHdr(sampleRgb, shallowColor, deepColor);
}

/**
 * W3-03: SSR + graded screen-space sky reflection.
 * Macro normal stabilizes base reflection; combined normal adds ripple distortion.
 */
vec3 waterComposeReflection(
        vec2 surfaceUv,
        vec3 worldPos,
        vec3 macroN,
        vec3 rippleN,
        vec3 V,
        vec3 baseColor,
        float roughness,
        vec3 shallowColor,
        vec3 deepColor,
        float detailStrength) {
    vec3 macroReflection = waterEvalReflectionFallback(surfaceUv, worldPos, macroN, V, shallowColor, deepColor);
    vec3 rippleReflection = waterEvalReflectionFallback(surfaceUv, worldPos, rippleN, V, shallowColor, deepColor);
    float grazing = 1.0 - max(dot(normalize(rippleN), normalize(V)), 0.0);
    float rippleMix = clamp(0.38 + detailStrength * 0.54 + grazing * 0.22, 0.38, 0.92);
    vec3 reflection = mix(macroReflection, rippleReflection, rippleMix);

    if (waterPush.ssrEnabled < 0.5) {
        return reflection;
    }

    WaterSsrHit ssrMacro = waterTraceScreenSpaceReflection(surfaceUv, worldPos, macroN, V, roughness);
    WaterSsrHit ssrRipple = waterTraceScreenSpaceReflection(surfaceUv, worldPos, rippleN, V, roughness);
    vec3 ssrColor = mix(
            waterGradeReflectionHdr(ssrMacro.color, shallowColor, deepColor),
            waterGradeReflectionHdr(ssrRipple.color, shallowColor, deepColor),
            rippleMix);
    float ssrWeight = clamp(max(ssrMacro.confidence, ssrRipple.confidence) * max(waterPush.ssrStrength, 0.0), 0.0, 1.0);
    return mix(reflection, ssrColor, ssrWeight);
}

#endif
