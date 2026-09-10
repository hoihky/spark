#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vWorldXZ;
layout(location = 2) in vec2 vScreenUv;
layout(location = 3) in vec4 vBaseColor;

layout(set = 0, binding = 1) uniform sampler2DArray sceneTextures;
layout(set = 0, binding = 11) uniform sampler2DArray sceneHdrTextures;

#include "scene_ubo.glsl"
#include "water_push.glsl"
#include "gerstner_wave.glsl"
#include "ibl.glsl"
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
    if (!gl_FrontFacing) {
        discard;
    }

    vec3 N = sparkGerstnerNormal(vWorldXZ, waterPush.timeSeconds, waterPush.waveCount, waterPush.waves);
    vec3 V = normalize(ubo.cameraPos.xyz - vWorldPos);
    float NdotV = max(dot(N, V), 0.001);

    vec3 baseColor = vBaseColor.rgb;
    float roughness = clamp(waterPush.roughness, 0.02, 1.0);

    vec3 Ld = normalize(ubo.lightDir.xyz);
    float NdotL = max(dot(N, Ld), 0.0);
    vec3 H = normalize(V + Ld);
    float NdotH = max(dot(N, H), 0.0);

    vec3 sunRad = ubo.lightColor.rgb * ubo.lightColor.w;
    vec3 ambient = ubo.ambientColor.rgb * baseColor * 0.35;

    int iblLayer = int(round(ubo.iblParams.x));
    bool hdrEnv = ubo.iblParams.z > 0.5;
    float iblIntensity = ubo.iblParams.y;
    vec2 envUvScale = sparkIblEnvLayerUvScale();
    vec3 R = reflect(-V, N);
    float specLod = sparkHdrSpecularEnvLod(roughness, 0.0);
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
    vec3 skyReflection = mix(skyDiffuse, skySpecular, 0.55) * iblIntensity;

    vec3 f0 = vec3(0.02);
    vec3 F = waterF_Schlick(NdotV, f0);
    float fresnel = clamp(F.x * 0.68 + 0.32, 0.0, 1.0);

    float D = waterD_GGX(NdotH, roughness);
    float G = waterG_SchlickGGX(NdotV, roughness) * waterG_SchlickGGX(NdotL, roughness);
    vec3 spec = D * G * F / max(4.0 * NdotV * NdotL, 1.0e-4) * sunRad;

    vec3 sunDiffuse = baseColor * sunRad * NdotL * 0.20;

    float alpha = clamp(waterPush.baseColor.a, 0.0, 1.0);
    float skyMix = mix(0.42, 0.62, alpha);
    vec3 shallow = mix(baseColor, skyReflection, skyMix + 0.10 * NdotL);
    vec3 body = mix(shallow, skyReflection, fresnel * (0.55 + 0.45 * alpha));

    vec3 color = body + sunDiffuse + spec + ambient * alpha;
    outColor = vec4(color, alpha);
}
