#ifndef SPARK_WATER_SSR_GLSL
#define SPARK_WATER_SSR_GLSL

#include "water_background.glsl"
#include "water_push.glsl"
#include "scene_ubo.glsl"

struct WaterSsrHit {
    vec3 color;
    float confidence;
};

float waterClipDepth(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    return clip.z / max(clip.w, 1.0e-5);
}

/** Half-res SSR: snap UV to 2×2 pixel bins to reduce texture bandwidth. */
vec2 waterSsrSampleUv(vec2 uv) {
    if (waterPush.ssrHalfRes < 0.5) {
        return uv;
    }
    vec2 fullSize = max(ubo.viewportSize.xy, vec2(1.0));
    vec2 halfStride = vec2(2.0) / fullSize;
    return (floor(uv / halfStride) + 0.5) * halfStride;
}

WaterSsrHit waterTraceScreenSpaceReflection(
        vec2 surfaceUv,
        vec3 worldPos,
        vec3 macroN,
        vec3 V,
        float roughness) {
    WaterSsrHit miss = WaterSsrHit(vec3(0.0), 0.0);
    if (waterPush.ssrEnabled < 0.5) {
        return miss;
    }

    vec3 safeN = normalize(macroN);
    vec3 safeV = normalize(V);
    vec3 R = reflect(-safeV, safeN);

    float horizonFade = smoothstep(waterPush.ssrHorizonFadeEnd, waterPush.ssrHorizonFadeStart, R.y);

    float maxDist = max(waterPush.ssrMaxRayDistance, 0.1);
    vec3 endWorld = worldPos + R * maxDist;
    vec2 uv0 = surfaceUv;
    vec2 uv1 = waterWorldToScreenUv(endWorld);
    if (uv0.x < 0.0 || uv0.x > 1.0 || uv0.y < 0.0 || uv0.y > 1.0) {
        return miss;
    }

    float z0 = waterClipDepth(worldPos);
    float z1 = waterClipDepth(endWorld);
    int maxSteps = max(waterPush.ssrMaxSteps, 1);
    float thickness = max(waterPush.ssrThickness, 1.0e-4);
    float stepScale = max(waterPush.ssrStepScale, 0.1);

    for (int i = 1; i <= maxSteps; ++i) {
        float t = float(i) / float(maxSteps);
        t = pow(t, stepScale);
        vec2 uv = waterSsrSampleUv(mix(uv0, uv1, t));
        if (uv.x < 0.001 || uv.x > 0.999 || uv.y < 0.001 || uv.y > 0.999) {
            break;
        }

        float rayZ = mix(z0, z1, t);
        float sceneDepth = waterSampleSceneDepthAtUv(uv);
        if (sceneDepth >= WATER_SCENE_DEPTH_SKY) {
            continue;
        }

        vec3 hitWorld = waterReconstructWorld(uv, sceneDepth);
        float hitZ = waterClipDepth(hitWorld);
        float depthDelta = hitZ - rayZ;
        float thicknessScale = thickness * (1.0 + roughness * 4.0);
        if (depthDelta <= thicknessScale && depthDelta >= -thicknessScale) {
            vec3 hitColor = waterSampleRefractOpaqueAtUv(uv);
            float distFade = 1.0 - t;
            float roughFade = 1.0 - clamp(roughness * 1.15, 0.0, 0.95);
            float confidence = distFade * roughFade * horizonFade * clamp(waterPush.ssrStrength, 0.0, 1.0);
            return WaterSsrHit(hitColor, confidence);
        }

        if (depthDelta > thicknessScale * 2.0) {
            break;
        }
    }

    return miss;
}

#endif
