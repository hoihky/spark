#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vWorldXZ;
layout(location = 2) in vec2 vScreenUv;
layout(location = 3) in vec4 vBaseColor;

layout(set = 0, binding = 1) uniform sampler2DArray sceneTextures;
layout(set = 0, binding = 11) uniform sampler2DArray sceneHdrTextures;

#include "scene_ubo.glsl"
#include "water_background.glsl"
#include "water_push.glsl"
#include "water_sun_shadow.glsl"
#include "water_detail.glsl"
#include "water_foam.glsl"
#include "water_ssr.glsl"
#include "water_reflection.glsl"
#include "gerstner_wave.glsl"
#include "color_space.glsl"

layout(location = 0) out vec4 outColor;

float waterD_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(SPARK_PI * denom * denom, 1.0e-6);
}

float waterG_SchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1.0e-6);
}

vec3 waterF_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 gerstnerN = sparkGerstnerNormal(vWorldXZ, waterPush.timeSeconds, waterPush.waveCount, waterPush.waves);
    float detailStrength = clamp(waterPush.detailNormalStrength, 0.0, 1.0);
    vec3 detailN = waterDetailNormal(vWorldXZ, waterPush.timeSeconds, detailStrength);
    vec3 N = waterCombineNormals(gerstnerN, detailN, detailStrength);
    vec3 V = normalize(ubo.cameraPos.xyz - vWorldPos);
    float NdotV = max(dot(N, V), 0.001);

    vec3 baseColor = vBaseColor.rgb;
    float roughness = clamp(waterPush.roughness, 0.02, 1.0);

    vec3 Ld = normalize(ubo.lightDir.xyz);
    float NdotL = max(dot(N, Ld), 0.0);
    vec3 H = normalize(V + Ld);
    float NdotH = max(dot(N, H), 0.0);
    float sunGracing = 1.0 - clamp(dot(N, Ld), 0.0, 1.0);

    float sunShadow = 1.0;
    if (ubo.shadowParams.w > 0.5 && (waterPush.shadowFlags & 2) != 0) {
        sunShadow = waterBlendSunShadow(vWorldPos, N, Ld);
        float shadowFadeEnd = ubo.viewportSize.z;
        if (shadowFadeEnd > 0.5) {
            float distCam = length(vWorldPos - ubo.cameraPos.xyz);
            float fadeStart = shadowFadeEnd * clamp(ubo.timeGlobal.y, 0.5, 0.98);
            float fade = smoothstep(fadeStart, shadowFadeEnd, distCam);
            sunShadow = mix(sunShadow, 1.0, fade);
        }
    }

    vec3 sunRad = ubo.lightColor.rgb * ubo.lightColor.w;
    vec3 ambient = ubo.ambientColor.rgb * baseColor * 0.20;

    vec3 f0 = vec3(0.02);
    vec3 F = waterF_Schlick(NdotV, f0);

    float D = waterD_GGX(NdotH, roughness);
    float G = waterG_SchlickGGX(NdotV, roughness) * waterG_SchlickGGX(NdotL, roughness);
    vec3 spec = D * G * F / max(4.0 * NdotV * NdotL, 1.0e-4) * sunRad * sunShadow;

    vec3 sunDiffuse = baseColor * sunRad * NdotL * 0.16 * sunShadow;

    vec2 surfaceUv = waterFramebufferScreenUv();
    float sceneDepthAtSurface = waterSampleSceneDepthAtUv(surfaceUv);
    if (!waterSceneDepthIsSky(sceneDepthAtSurface)) {
        float sceneSurfaceY = waterReconstructWorld(surfaceUv, sceneDepthAtSurface).y;
        if (sceneSurfaceY > vWorldPos.y + 0.25) {
            discard;
        }
    }

    float columnDepth = waterComputeColumnDepth(surfaceUv, vWorldPos.y);
    vec3 shallowColor = mix(baseColor * 1.35, baseColor * 0.85, 0.35);
    vec3 deepColor = waterPush.deepColor.rgb;
    float absorption = max(waterPush.absorption, 0.0);

    vec3 oceanAnchor = waterOpenOceanTint(shallowColor, deepColor, absorption);
    float shoreBlend = waterShoreBlendWeight(columnDepth);

    vec3 waterTint = waterApplyDepthAbsorption(shallowColor, deepColor, max(columnDepth, 0.12), absorption);
    float underwater = smoothstep(0.05, 0.35, columnDepth);
    vec2 refractUv = waterComputeRefractScreenUv(surfaceUv, vWorldPos, N, V, roughness);
    vec3 refractedScene = waterSampleRefractOpaqueAtUv(refractUv);
    vec3 shoreRefracted = refractedScene * mix(vec3(1.0), waterTint, mix(0.55, 0.38, underwater));
    shoreRefracted += baseColor * mix(0.14, 0.08, underwater);

    vec3 reflection = waterComposeReflection(
            surfaceUv,
            vWorldPos,
            gerstnerN,
            N,
            V,
            baseColor,
            roughness,
            shallowColor,
            deepColor,
            detailStrength);

    float macroNdotV = max(dot(gerstnerN, V), 0.001);
    float bodyFresnel = waterComputeBodyFresnel(macroNdotV);

    // Open ocean: HDR refraction (ripple-distorted) vs sky reflection — fresnel drives the mix.
    vec3 openRefract = mix(
            waterGradeRefractHdr(refractedScene, shallowColor, deepColor),
            oceanAnchor,
            0.18);
    vec3 openBody = mix(openRefract, reflection, bodyFresnel);
    vec3 shoreBody = mix(shoreRefracted, reflection, bodyFresnel);
    vec3 body = mix(openBody, shoreBody, shoreBlend);

    float alpha = clamp(waterPush.baseColor.a, 0.0, 1.0);

    float crestFoam = waterCrestFoamMask(
            vWorldXZ,
            waterPush.timeSeconds,
            waterPush.waveCount,
            waterPush.waves,
            clamp(waterPush.foamStrength, 0.0, 1.0),
            gerstnerN,
            V);
    float shorelineFoam = waterShorelineFoamMask(
            surfaceUv,
            vWorldPos.y,
            max(waterPush.shorelineFoamMaxDepth, 0.05),
            clamp(waterPush.shorelineFoamStrength, 0.0, 1.0),
            waterPush.timeSeconds,
            vWorldXZ);
    float foam = clamp(max(crestFoam, shorelineFoam), 0.0, 1.0);

    float rippleVis = 1.0 + 0.10 * (1.0 - N.y) + 0.10 * (1.0 - gerstnerN.y);
    vec3 color = body * rippleVis + sunDiffuse + spec * (1.0 - crestFoam * 0.75) * 1.42 + ambient * alpha;

    float skyFill = (1.0 - sunShadow) * (0.05 + 0.09 * sunGracing);
    vec3 skyHue = mix(ubo.ambientColor.rgb, ubo.ambientSky.rgb, 0.70);
    color += baseColor * skyFill * max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * skyHue *
             max(length(sunRad), 1.0e-4) * 0.10;

    color = mix(color, vec3(0.82, 0.90, 0.96), foam * 0.62);

    outColor = vec4(color, alpha);
}
