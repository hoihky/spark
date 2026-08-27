// Image-based lighting: equirect environment + split-sum specular (metals).
#ifndef SPARK_IBL_GLSL
#define SPARK_IBL_GLSL

#include "color_space.glsl"
#include "equirect.glsl"

layout(set = 0, binding = 12) uniform sampler2D iblBrdfLut;

const float SPARK_PI = 3.14159265359;
const uint kIblSampleCount = 32u;
// Matches 1024² scene HDR layers (VulkanSceneHdrTextureUploader::kLayerSize).
const float SPARK_HDR_ENV_MAX_LOD = 10.0;
const float SPARK_HDR_DIFFUSE_IBL_LOD = 3.0;
const float SPARK_HDR_IBL_TONE_KNEE = 3.2;

vec3 sparkClampIblFireflies(vec3 rgb) {
    const float kMaxLuminance = 5.5;
    float lum = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    return rgb * min(1.0, kMaxLuminance / max(lum, 1e-4));
}

// Bring HDR equirect samples into the same linear range as direct lights.
vec3 sparkToneMapHdrIblSample(vec3 rgb) {
    float lum = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    float scale = SPARK_HDR_IBL_TONE_KNEE / max(lum + SPARK_HDR_IBL_TONE_KNEE, 1e-4);
    return sparkClampIblFireflies(rgb * scale);
}

float sparkHdrSpecularEnvLod(float roughness, float metallic) {
    float r = clamp(roughness, 0.0, 1.0);
    float lod = r * r * SPARK_HDR_ENV_MAX_LOD;
    // Dielectrics: blur env reflections heavily to avoid HDR texel fireflies on ground/walls.
    if (metallic < 0.5) {
        lod = max(lod, 6.0);
    }
    return clamp(lod, 0.0, SPARK_HDR_ENV_MAX_LOD);
}

vec3 sparkSampleEquirect(
        sampler2DArray ldrTex,
        sampler2DArray hdrTex,
        vec3 dir,
        int layer,
        bool hdr,
        vec2 layerUvScale,
        float sampleLod) {
    vec2 uv = sparkEquirectDirectionUv(dir, layerUvScale);
    if (hdr) {
        int hl = clamp(layer, 0, 7);
        vec3 rgb = textureLod(hdrTex, vec3(uv.x, uv.y, float(hl)), sampleLod).rgb;
        return sparkToneMapHdrIblSample(rgb);
    }
    int ll = clamp(layer, 0, 63);
    return sparkClampIblFireflies(sparkSrgbToLinear(textureLod(ldrTex, vec3(uv.x, uv.y, float(ll)), 0.0).rgb));
}

vec3 sparkSampleProceduralEnv(vec3 dir) {
    vec3 horizon = ubo.ambientColor.rgb;
    vec3 zenith = ubo.ambientSky.rgb;
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    return mix(horizon, zenith, pow(t, 0.65));
}

vec2 sparkIblEnvLayerUvScale() {
    return vec2(ubo.lightDir.w, ubo.ambientSky.w);
}

vec3 sparkSampleEnvironment(
        sampler2DArray ldrTex,
        sampler2DArray hdrTex,
        vec3 dir,
        int layer,
        bool hdr,
        vec2 layerUvScale,
        float sampleLod) {
    if (layer >= 0) {
        return sparkSampleEquirect(ldrTex, hdrTex, dir, layer, hdr, layerUvScale, sampleLod);
    }
    return sparkSampleProceduralEnv(dir);
}

vec2 sparkEnvBrdfApprox(float roughness, float nDotV) {
    vec2 uv = vec2(clamp(nDotV, 0.0, 1.0), clamp(roughness, 0.0, 1.0));
    return texture(iblBrdfLut, uv).rg;
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

vec3 sparkPrefilterEnvironment(
        sampler2DArray ldrTex,
        sampler2DArray hdrTex,
        vec3 r,
        float roughness,
        float metallic,
        int layer,
        bool hdr,
        vec2 layerUvScale) {
    float specLod = hdr ? sparkHdrSpecularEnvLod(roughness, metallic) : 0.0;
    if (roughness <= 0.04) {
        return sparkSampleEnvironment(ldrTex, hdrTex, r, layer, hdr, layerUvScale, specLod);
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
            vec3 sampleRgb = sparkSampleEnvironment(ldrTex, hdrTex, l, layer, hdr, layerUvScale, specLod);
            prefiltered += sampleRgb * nDotL;
            totalWeight += nDotL;
        }
    }
    return prefiltered / max(totalWeight, 1e-4);
}

vec3 sparkEvalSpecularIbl(
        sampler2DArray ldrTex,
        sampler2DArray hdrTex,
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
    bool hdrEnv = ubo.iblParams.z > 0.5;
    vec2 envUvScale = sparkIblEnvLayerUvScale();
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    vec3 r = reflect(-v, n);
    float nDotV = max(dot(n, v), 0.001);
    vec3 prefiltered = sparkPrefilterEnvironment(ldrTex, hdrTex, r, roughness, metallic, layer, hdrEnv, envUvScale);
    vec2 brdf = sparkEnvBrdfApprox(roughness, nDotV);
    float dielectricAtten = mix(0.55, 1.0, smoothstep(0.02, 0.55, metallic));
    return prefiltered * (f0 * brdf.x + brdf.y) * intensity * occlusion * dielectricAtten;
}

vec3 sparkEvalDiffuseIbl(
        sampler2DArray ldrTex,
        sampler2DArray hdrTex,
        vec3 n,
        vec3 baseColor,
        float metallic,
        float occlusion) {
    if (ubo.iblParams.w < 0.5) {
        return vec3(0.0);
    }
    int layer = int(round(ubo.iblParams.x));
    float intensity = ubo.iblParams.y;
    bool hdrEnv = ubo.iblParams.z > 0.5;
    vec2 envUvScale = sparkIblEnvLayerUvScale();
    float sampleLod = hdrEnv ? SPARK_HDR_DIFFUSE_IBL_LOD : 0.0;
    vec3 irradiance = sparkSampleEnvironment(ldrTex, hdrTex, n, layer, hdrEnv, envUvScale, sampleLod);
    return irradiance * baseColor * (1.0 - metallic) * intensity * occlusion;
}

#endif
