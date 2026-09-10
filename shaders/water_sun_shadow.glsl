#ifndef SPARK_WATER_SUN_SHADOW_GLSL
#define SPARK_WATER_SUN_SHADOW_GLSL

#include "scene_ubo.glsl"

layout(set = 0, binding = 3) uniform sampler2D shadowMap;

const float SPARK_WATER_PI = 3.14159265359;
const float kWaterShadowPcfW5[5] = float[](0.0625, 0.25, 0.375, 0.25, 0.0625);

float waterShadowPcfRotation01(vec2 fragPx) {
    return fract(sin(dot(fragPx, vec2(12.9898, 78.233))) * 43758.5453);
}

float waterShadowCascadeViewDepth(vec3 worldPos) {
    vec4 worldNear = ubo.invViewProj * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 worldMid = ubo.invViewProj * vec4(0.0, 0.0, 0.5, 1.0);
    worldNear.xyz /= worldNear.w;
    worldMid.xyz /= worldMid.w;
    vec3 viewForward = normalize(worldMid.xyz - worldNear.xyz);
    return max(dot(worldPos - ubo.cameraPos.xyz, viewForward), 0.0);
}

int waterSelectShadowCascade(float viewDepth) {
    if (viewDepth <= ubo.cascadeSplits.x) {
        return 0;
    }
    if (viewDepth <= ubo.cascadeSplits.y) {
        return 1;
    }
    if (viewDepth <= ubo.cascadeSplits.z) {
        return 2;
    }
    return 3;
}

vec2 waterCascadeAtlasUv(vec2 uv01, int cascade) {
    vec4 a = ubo.cascadeAtlas[cascade];
    return uv01 * a.zw + a.xy;
}

float waterSampleSunShadowPcf(
        vec2 uvCenter,
        float refZ,
        vec2 texel,
        float penumbraScale,
        float rot01) {
    float c = cos(rot01 * SPARK_WATER_PI * 2.0);
    float s = sin(rot01 * SPARK_WATER_PI * 2.0);
    mat2 rot = mat2(c, -s, s, c);
    float acc = 0.0;
    float wsum = 0.0;
    vec2 margin = texel * max(3.0, penumbraScale * 2.5);
    vec2 uvC = clamp(uvCenter, margin, vec2(1.0) - margin);
    for (int j = -2; j <= 2; ++j) {
        for (int i = -2; i <= 2; ++i) {
            float w = kWaterShadowPcfW5[i + 2] * kWaterShadowPcfW5[j + 2];
            vec2 off = rot * (vec2(float(i), float(j)) * texel * penumbraScale);
            vec2 suv = clamp(uvC + off, vec2(0.0), vec2(1.0));
            float mapZ = textureLod(shadowMap, suv, 0.0).r;
            acc += w * (refZ <= mapZ ? 1.0 : 0.0);
            wsum += w;
        }
    }
    return acc / max(wsum, 1e-5);
}

float waterSampleSunShadowAtCascade(int cascade, vec3 worldPos, vec3 N, vec3 Ld, float sunGracing) {
    vec4 ls = ubo.worldToShadowClip[cascade] * vec4(worldPos, 1.0);
    vec3 proj = ls.xyz / max(ls.w, 1e-5);
    vec2 uv = vec2(proj.x * 0.5 + 0.5, proj.y * 0.5 + 0.5);
    uv = waterCascadeAtlasUv(uv, cascade);
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    float g = sunGracing;
    float depthBias = max(ubo.shadowParams.x, 1e-5) * (1.0 + 6.25 * g);
    depthBias += ubo.shadowParams.y * 0.22 * g;
    float refZ = clamp(proj.z - depthBias, 0.0, 1.0);
    vec2 texel = vec2(ubo.shadowParams.z);
    float penumbraScale = clamp(1.0 + 2.75 * g, 1.0, 2.85);
    float rot01 = waterShadowPcfRotation01(gl_FragCoord.xy);
    float sh = waterSampleSunShadowPcf(uv, refZ, texel, penumbraScale, rot01);
    return pow(clamp(sh, 0.0, 1.0), 1.08);
}

float waterBlendSunShadow(vec3 worldPos, vec3 N, vec3 Ld) {
    float sunGracing = 1.0 - clamp(dot(N, Ld), 0.0, 1.0);
    float viewDepth = waterShadowCascadeViewDepth(worldPos);
    int cascade = waterSelectShadowCascade(viewDepth);
    float sh = waterSampleSunShadowAtCascade(cascade, worldPos, N, Ld, sunGracing);

    float blendFrac = clamp(ubo.timeGlobal.w, 0.001, 0.35);
    const float kMinBlendM = 0.35;
    if (cascade > 0) {
        float splitLow = ubo.cascadeSplits[cascade - 1];
        float blendLen = max(splitLow * blendFrac, kMinBlendM);
        float wPrev = 1.0 - smoothstep(splitLow - blendLen, splitLow, viewDepth);
        if (wPrev > 1e-4) {
            float shPrev = waterSampleSunShadowAtCascade(cascade - 1, worldPos, N, Ld, sunGracing);
            sh = mix(sh, shPrev, wPrev);
        }
    }
    if (cascade < 3) {
        float splitHigh = ubo.cascadeSplits[cascade];
        float blendLen = max(splitHigh * blendFrac, kMinBlendM);
        float wNext = smoothstep(splitHigh, splitHigh + blendLen, viewDepth);
        if (wNext > 1e-4) {
            float shNext = waterSampleSunShadowAtCascade(cascade + 1, worldPos, N, Ld, sunGracing);
            sh = mix(sh, shNext, wNext);
        }
    }
    return sh;
}

#endif
