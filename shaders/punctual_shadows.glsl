#ifndef SPARK_PUNCTUAL_SHADOWS_GLSL
#define SPARK_PUNCTUAL_SHADOWS_GLSL

#include "clustered_lights.glsl"

const int kMaxSpotShadowMaps = 4;
const int kMaxPointShadowMaps = 2;

layout(std430, set = 0, binding = 6) readonly buffer PunctualShadowSSBO {
    uint numSpotShadows;
    uint numPointShadows;
    uint punctualShadowEnabled;
    uint _psPad0;
    int spotLightIndex[kMaxSpotShadowMaps];
    int pointLightIndex[kMaxPointShadowMaps];
    int _psPad1[2];
    mat4 spotWorldToClip[kMaxSpotShadowMaps];
    vec4 spotAtlas[kMaxSpotShadowMaps];
    vec4 pointPosRange[kMaxPointShadowMaps];
    uint pointBaseLayer[kMaxPointShadowMaps];
    uint _psPad2[2];
    mat4 pointFaceWorldToClip[kMaxPointShadowMaps][6];
    int pointShadowSlotByLight[kMaxClusteredPointLights];
    int spotShadowSlotByLight[kMaxClusteredSpotLights];
} punctualShadow;

layout(set = 0, binding = 7) uniform sampler2D spotShadowMap;
layout(set = 0, binding = 8) uniform sampler2DArray pointShadowMap;

float samplePunctualShadowPcf3(sampler2D map, vec2 uvCenter, float refZ, vec2 texel, float rot01) {
    float c = cos(rot01 * 6.28318530718);
    float s = sin(rot01 * 6.28318530718);
    mat2 rot = mat2(c, -s, s, c);
    float acc = 0.0;
    float wsum = 0.0;
    vec2 margin = texel * 2.0;
    vec2 uvC = clamp(uvCenter, margin, vec2(1.0) - margin);
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            float w = 1.0;
            vec2 off = rot * (vec2(float(i), float(j)) * texel);
            vec2 suv = clamp(uvC + off, vec2(0.0), vec2(1.0));
            float mapZ = textureLod(map, suv, 0.0).r;
            acc += w * (refZ <= mapZ ? 1.0 : 0.0);
            wsum += w;
        }
    }
    return acc / max(wsum, 1e-5);
}

float sampleSpotShadowAtSlot(int slot, vec3 worldPos, vec3 N, vec3 L) {
    vec4 ls = punctualShadow.spotWorldToClip[slot] * vec4(worldPos, 1.0);
    if (ls.w <= 1e-5) {
        return 1.0;
    }
    vec3 proj = ls.xyz / ls.w;
    if (abs(proj.x) > 1.0 || abs(proj.y) > 1.0 || proj.z < 0.0 || proj.z > 1.0) {
        return 1.0;
    }
    vec2 uv = vec2(proj.x * 0.5 + 0.5, proj.y * 0.5 + 0.5);
    vec4 atlas = punctualShadow.spotAtlas[slot];
    uv = uv * atlas.zw + atlas.xy;
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    float gracing = 1.0 - clamp(dot(N, L), 0.0, 1.0);
    float depthBias = max(ubo.shadowParams.x, 1e-5) * (1.0 + 4.0 * gracing);
    depthBias += ubo.shadowParams.y * 0.18 * gracing;
    float refZ = clamp(proj.z - depthBias, 0.0, 1.0);
    vec2 texel = vec2(1.0 / 512.0);
    float rot01 = shadowPcfRotation01(gl_FragCoord.xy);
    return samplePunctualShadowPcf3(spotShadowMap, uv, refZ, texel, rot01);
}

int selectPointShadowFace(int slot, vec3 worldPos) {
    vec3 toFrag = worldPos - punctualShadow.pointPosRange[slot].xyz;
    vec3 a = abs(toFrag);
    if (a.x >= a.y && a.x >= a.z) {
        return toFrag.x > 0.0 ? 0 : 1;
    }
    if (a.y >= a.z) {
        return toFrag.y > 0.0 ? 2 : 3;
    }
    return toFrag.z > 0.0 ? 4 : 5;
}

float samplePointShadowAtSlot(int slot, vec3 worldPos, vec3 N, vec3 L) {
    vec3 lightPos = punctualShadow.pointPosRange[slot].xyz;
    float range = max(punctualShadow.pointPosRange[slot].w, 1e-3);
    vec3 toFrag = worldPos - lightPos;
    float dist = length(toFrag);
    if (dist > range) {
        return 1.0;
    }

    int face = selectPointShadowFace(slot, worldPos);
    mat4 wtc = punctualShadow.pointFaceWorldToClip[slot][face];
    vec4 ls = wtc * vec4(worldPos, 1.0);
    if (ls.w <= 1e-5) {
        return 1.0;
    }
    vec3 proj = ls.xyz / ls.w;
    if (abs(proj.x) > 1.0 || abs(proj.y) > 1.0 || proj.z < 0.0 || proj.z > 1.0) {
        return 1.0;
    }

    vec2 uv = vec2(proj.x * 0.5 + 0.5, proj.y * 0.5 + 0.5);
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }

    float gracing = 1.0 - clamp(dot(N, L), 0.0, 1.0);
    float depthBias = max(ubo.shadowParams.x, 1e-5) * (1.0 + 3.5 * gracing);
    depthBias += ubo.shadowParams.y * 0.15 * gracing;
    float refZ = clamp(proj.z - depthBias, 0.0, 1.0);
    uint layer = punctualShadow.pointBaseLayer[slot] + uint(face);
    vec2 texel = vec2(1.0 / 512.0);
    float rot01 = shadowPcfRotation01(gl_FragCoord.xy);
    float c = cos(rot01 * 6.28318530718);
    float s = sin(rot01 * 6.28318530718);
    mat2 rot = mat2(c, -s, s, c);
    float acc = 0.0;
    float wsum = 0.0;
    vec2 margin = texel * 2.0;
    vec2 uvC = clamp(uv, margin, vec2(1.0) - margin);
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            vec2 off = rot * (vec2(float(i), float(j)) * texel);
            vec2 suv = clamp(uvC + off, vec2(0.0), vec2(1.0));
            float mapZ = textureLod(pointShadowMap, vec3(suv, float(layer)), 0.0).r;
            acc += (refZ <= mapZ ? 1.0 : 0.0);
            wsum += 1.0;
        }
    }
    return acc / max(wsum, 1e-5);
}

#endif
