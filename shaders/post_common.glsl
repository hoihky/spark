#ifndef SPARK_POST_COMMON_GLSL
#define SPARK_POST_COMMON_GLSL

#include "scene_ubo.glsl"

layout(set = 0, binding = 1) uniform sampler2D sceneColorTex;
layout(set = 0, binding = 2) uniform sampler2D sceneDepthTex;

layout(push_constant) uniform PostPush {
    float ssaoEnabled;
    float ssaoRadius;
    float ssaoBias;
    float ssaoStrength;
    float depthFlipV;
    float pad0;
    float pad1;
    float pad2;
} post;

vec2 depthSampleUv(vec2 uv) {
    if (post.depthFlipV > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    return uv;
}

float sampleSceneDepth(vec2 uv) {
    return texture(sceneDepthTex, depthSampleUv(uv)).r;
}

// Match PerspectiveVulkan + clustered tile mapping: screenUv.y = (1 - ndcY) * 0.5.
vec2 ndcToScreenUv(vec2 ndc) {
    return vec2(ndc.x * 0.5 + 0.5, (1.0 - ndc.y) * 0.5);
}

vec2 screenUvToNdc(vec2 uv) {
    return vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
}

vec3 reconstructWorldPos(vec2 uv, float depthSample) {
    vec2 ndc = screenUvToNdc(uv);
    vec4 clip = vec4(ndc, depthSample, 1.0);
    vec4 world = ubo.invViewProj * clip;
    return world.xyz / max(world.w, 1e-5);
}

vec2 projectWorldToScreenUv(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    if (clip.w <= 0.0) {
        return vec2(-1.0);
    }
    vec3 ndc = clip.xyz / clip.w;
    return ndcToScreenUv(ndc.xy);
}

vec3 viewNormalFromDepth(vec2 uv, vec2 texelSize) {
    float depthC = sampleSceneDepth(uv);
    if (depthC >= 0.9999) {
        return vec3(0.0, 0.0, 1.0);
    }
    vec3 posC = reconstructWorldPos(uv, depthC);
    vec3 posR = reconstructWorldPos(uv + vec2(texelSize.x, 0.0), sampleSceneDepth(uv + vec2(texelSize.x, 0.0)));
    vec3 posU = reconstructWorldPos(uv + vec2(0.0, texelSize.y), sampleSceneDepth(uv + vec2(0.0, texelSize.y)));
    vec3 dx = posR - posC;
    vec3 dy = posU - posC;
    vec3 n = cross(dx, dy);
    float len2 = dot(n, n);
    if (len2 < 1e-8) {
        return vec3(0.0, 1.0, 0.0);
    }
    return normalize(n);
}

#endif
