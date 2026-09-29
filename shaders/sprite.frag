#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 vTex;
layout(location = 8) in vec2 vNormalTex;
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
#include "sprite_lighting_2d.glsl"

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

    if (vLightingMode == 5) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec3 lit = sparkSpriteAccumDirectional(base.rgb, n, vLightingA.y, vLightingA.z);
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, vLightingA.w);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 6) {
        int normalLayer = int(vLightingB.x);
        int rampLayer = int(vLightingB.y);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec3 ldir = normalize(vec3(ubo.lightDir.xy, 0.0));
        float ndl = clamp(dot(n, ldir) * 0.5 + 0.5, 0.0, 1.0);
        ndl = pow(ndl, max(0.25, vLightingA.y));
        vec3 ramp = vec3(ndl);
        if (rampLayer >= 0) {
            ramp = textureLod(spriteSceneTextures, vec3(ndl, 0.5, float(rampLayer)), 0.0).rgb;
        }
        vec3 lit = base.rgb * mix(vec3(vLightingA.z), ramp, 0.85);
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.65);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 7) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec3 lit = sparkSpriteAccumDirectional(base.rgb, n, 0.28, 0.75);
        vec3 ldir = normalize(vec3(ubo.lightDir.xy, 0.35));
        vec3 h = normalize(ldir + vec3(0.0, 0.0, 1.0));
        float spec = pow(max(0.0, dot(n, h)), max(1.0, vLightingA.y)) * max(0.0, vLightingA.z);
        lit += ubo.lightColor.rgb * ubo.lightColor.w * spec;
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.55);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 8) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        float skyMix = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 ground = vLightingA.rgb;
        vec3 sky = ubo.ambientSky.rgb;
        vec3 hemi = mix(ground, sky, skyMix * clamp(vLightingA.w, 0.0, 1.0));
        vec3 lit = base.rgb * (hemi + 0.25);
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.45);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 9) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec3 ldir = normalize(vec3(ubo.lightDir.xy, 0.0));
        float wrap = clamp(vLightingA.y, 0.0, 1.0);
        float ndl = clamp((dot(n, ldir) + wrap) / (1.0 + wrap), 0.0, 1.0);
        vec3 amb = ubo.ambientColor.rgb * 0.35;
        vec3 sun = ubo.lightColor.rgb * ubo.lightColor.w * ndl * max(0.0, vLightingA.z);
        vec3 lit = base.rgb * (amb + sun);
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.7);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 10) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec3 lit = sparkSpriteAccumDirectional(base.rgb, n, 0.3, 0.85);
        float rim = pow(1.0 - max(0.0, n.z), max(0.5, vLightingA.y)) * max(0.0, vLightingA.z);
        lit += vec3(vLightingB.y, vLightingB.z, vLightingB.w) * rim;
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.55);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 11) {
        int normalLayer = int(vLightingB.x);
        int rampLayer = int(vLightingB.y);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        vec2 mc = clamp(n.xy * 0.5 + 0.5, 0.0, 1.0);
        vec3 mat = vec3(mc, 0.5);
        if (rampLayer >= 0) {
            mat = textureLod(spriteSceneTextures, vec3(mc, float(rampLayer)), 0.0).rgb;
        }
        vec3 lit = base.rgb * mat;
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, 0.35);
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 12) {
        int normalLayer = int(vLightingB.x);
        vec3 n = sparkSpriteDecodeNormal(vNormalTex, normalLayer, vLightingA.x);
        float flicker = 1.0 - clamp(vLightingB.y, 0.0, 0.85) * (0.5 + 0.5 * sin(ubo.timeGlobal.x * max(0.5, vLightingA.w) * 6.2831853));
        vec3 lit = sparkSpriteAccumDirectional(base.rgb, n, vLightingA.y, vLightingA.z) * flicker;
        lit += sparkSpriteAccumPointLights(base.rgb, n, vWorldPos, vLightingA.w * 0.65) * flicker;
        outColor = vec4(lit, base.a);
        return;
    }

    if (vLightingMode == 13) {
        float decay = max(0.01, vLightingB.x);
        float t = exp(-ubo.timeGlobal.x / decay);
        vec3 flash = vLightingA.rgb * max(0.0, vLightingA.w) * t;
        outColor = vec4(base.rgb + flash, base.a);
        return;
    }

    if (vLightingMode == 14) {
        float w = max(1.0, vLightingA.w);
        float soft = max(0.5, vLightingB.x);
        vec2 px = vec2(1.0 / 256.0, 1.0 / 256.0) * w;
        float a0 = base.a;
        float a1 = textureLod(spriteSceneTextures, vec3(vTex + vec2(px.x, 0.0), float(vLayer)), 0.0).a;
        float a2 = textureLod(spriteSceneTextures, vec3(vTex - vec2(px.x, 0.0), float(vLayer)), 0.0).a;
        float a3 = textureLod(spriteSceneTextures, vec3(vTex + vec2(0.0, px.y), float(vLayer)), 0.0).a;
        float a4 = textureLod(spriteSceneTextures, vec3(vTex - vec2(0.0, px.y), float(vLayer)), 0.0).a;
        float edge = clamp((a1 + a2 + a3 + a4) - a0 * 4.0, 0.0, 1.0);
        edge = pow(edge, 1.0 / soft);
        vec3 col = mix(base.rgb, vLightingA.rgb, edge);
        outColor = vec4(col, max(base.a, edge * vLightingA.a));
        return;
    }

    if (vLightingMode == 15) {
        float edge = clamp(vLightingA.x, 0.0, 1.0);
        float noise = fract(sin(dot(vLocalXY * max(1.0, vLightingA.y) + ubo.timeGlobal.x * vLightingA.w, vec2(12.9898, 78.233))) * 43758.5453);
        float soft = max(0.001, vLightingA.z);
        float clip = smoothstep(edge - soft, edge + soft, noise);
        if (clip < 0.5) {
            discard;
        }
        outColor = base;
        return;
    }

    outColor = base;
}
