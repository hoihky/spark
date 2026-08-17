#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 vTex;
layout(location = 1) flat in int vLayer;
layout(location = 2) in vec2 vLocalXY;
layout(location = 3) in vec3 vWorldPos;
layout(location = 4) flat in vec4 vTint;
layout(location = 5) flat in int vLightingMode;
layout(location = 6) flat in vec4 vLightingA;
layout(location = 7) flat in vec4 vLightingB;

layout(set = 0, binding = 10) uniform sampler2DArray spriteSceneTextures;

#include "scene_ubo.glsl"
#include "clustered_lights.glsl"
#include "color_space.glsl"

layout(location = 0) out vec4 outColor;

vec4 sampleBase() {
    if (vLayer < 0) {
        return vTint;
    }
    vec4 tex = textureLod(spriteSceneTextures, vec3(vTex, float(vLayer)), 0.0);
    tex.rgb = sparkSrgbToLinear(tex.rgb);
    return tex * vTint;
}

float pulse01(float hz) {
    float t = ubo.timeGlobal.x * hz * 6.2831853;
    return 0.5 + 0.5 * sin(t);
}

void main() {
    vec4 base = sampleBase();
    if (base.a < 1e-4) {
        discard;
    }

    if (vLightingMode == 0) {
        outColor = base;
        return;
    }

    if (vLightingMode == 1) {
        vec2 l2 = normalize(ubo.lightDir.xy + vec2(1e-5));
        vec2 n2 = normalize(vLocalXY + vec2(1e-5));
        float ndl = max(0.0, dot(n2, l2));
        float amb = clamp(vLightingA.x, 0.0, 1.0);
        float dif = max(vLightingA.y, 0.0);
        vec3 lit = base.rgb * (amb + dif * ndl);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 2) {
        float r = length(vLocalXY) * 2.0;
        float rim = pow(max(0.0, 1.0 - r), max(0.05, vLightingA.a));
        vec3 rimc = vLightingA.rgb * rim * max(0.0, vLightingB.x);
        outColor = vec4(base.rgb + rimc, base.a);
        return;
    }

    if (vLightingMode == 3) {
        float hz = max(0.01, vLightingA.w);
        float pulse = pulse01(hz);
        float str = max(0.0, vLightingB.x);
        float mixb = clamp(vLightingB.y, 0.0, 1.0);
        vec3 emit = vLightingA.rgb * pulse * str;
        vec3 col = base.rgb * mix(1.0, 0.55 + 0.45 * pulse, mixb) + emit;
        outColor = vec4(col, base.a);
        return;
    }

    if (vLightingMode == 4) {
        vec3 acc = base.rgb * clamp(vLightingA.x, 0.0, 1.0);
        float difs = max(0.0, vLightingA.y);
        if (ubo.clusterDepth.w >= 0.5) {
            uint numPt = min(clusterLights.numPointLights, kMaxClusteredPointLights);
            for (uint i = 0u; i < numPt; ++i) {
                vec3 p = clusterLights.pointPositionRange[i].xyz;
                float range = max(clusterLights.pointPositionRange[i].w, 1e-3);
                vec3 lc = clusterLights.pointColorIntensity[i].xyz;
                float intens = clusterLights.pointColorIntensity[i].w;
                float d = distance(vWorldPos, p);
                float att = sparkPointAttenuation(d, range);
                acc += base.rgb * lc * intens * att * difs;
            }
        }
        outColor = vec4(acc, base.a);
        return;
    }

    outColor = base;
}
