// Image-based lighting: equirect environment + split-sum specular (metals).
#ifndef SPARK_IBL_GLSL
#define SPARK_IBL_GLSL

#include "color_space.glsl"

const float SPARK_PI = 3.14159265359;
const uint kIblSampleCount = 16u;

vec3 sparkSampleEquirect(sampler2DArray tex, vec3 dir, int layer) {
    float phi = atan(dir.z, dir.x) / (2.0 * SPARK_PI) + 0.5;
    phi = fract(phi);
    float mu = acos(clamp(dir.y, -1.0, 1.0)) / SPARK_PI;
    float vEq = 1.0 - mu;
    return sparkSrgbToLinear(textureLod(tex, vec3(phi, vEq, float(layer)), 0.0).rgb);
}

vec3 sparkSampleProceduralEnv(vec3 dir) {
    vec3 horizon = ubo.ambientColor.rgb;
    vec3 zenith = ubo.ambientSky.rgb;
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    return mix(horizon, zenith, pow(t, 0.65));
}

vec3 sparkSampleEnvironment(sampler2DArray tex, vec3 dir, int layer) {
    if (layer >= 0) {
        return sparkSampleEquirect(tex, dir, layer);
    }
    return sparkSampleProceduralEnv(dir);
}

vec2 sparkEnvBrdfApprox(float roughness, float nDotV) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * nDotV)) * r.x + r.y;
    vec2 ab = vec2(-1.0, 1.0) * a004 + r.zw;
    return clamp(ab, vec2(0.0), vec2(1.0));
}

vec3 sparkImportanceSampleGGX(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * SPARK_PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

vec2 sparkHammersley(uint i, uint n) {
    return vec2(float(i) / float(n), fract(float(i) * 0.6180339887));
}

vec3 sparkPrefilterEnvironment(sampler2DArray tex, vec3 r, float roughness, int layer) {
    if (roughness <= 0.04) {
        return sparkSampleEnvironment(tex, r, layer);
    }
    vec3 n = r;
    vec3 v = r;
    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;
    for (uint i = 0u; i < kIblSampleCount; ++i) {
        vec2 xi = sparkHammersley(i, kIblSampleCount);
        vec3 h = sparkImportanceSampleGGX(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);
        float nDotL = max(dot(n, l), 0.0);
        if (nDotL > 0.0) {
            prefiltered += sparkSampleEnvironment(tex, l, layer) * nDotL;
            totalWeight += nDotL;
        }
    }
    return prefiltered / max(totalWeight, 1e-4);
}

vec3 sparkEvalSpecularIbl(
        sampler2DArray tex,
        vec3 n,
        vec3 v,
        vec3 baseColor,
        float metallic,
        float roughness,
        float occlusion) {
    if (ubo.iblParams.w < 0.5) {
        return vec3(0.0);
    }
    int layer = int(round(ubo.iblParams.x));
    float intensity = ubo.iblParams.y;
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    vec3 r = reflect(-v, n);
    float nDotV = max(dot(n, v), 0.001);
    vec3 prefiltered = sparkPrefilterEnvironment(tex, r, roughness, layer);
    vec2 brdf = sparkEnvBrdfApprox(roughness, nDotV);
    return prefiltered * (f0 * brdf.x + brdf.y) * intensity * occlusion;
}

vec3 sparkEvalDiffuseIbl(
        sampler2DArray tex,
        vec3 n,
        vec3 baseColor,
        float metallic,
        float occlusion) {
    if (ubo.iblParams.w < 0.5) {
        return vec3(0.0);
    }
    int layer = int(round(ubo.iblParams.x));
    float intensity = ubo.iblParams.y;
    vec3 irradiance = sparkSampleEnvironment(tex, n, layer);
    return irradiance * baseColor * (1.0 - metallic) * intensity * occlusion;
}

#endif
