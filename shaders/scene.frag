#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vAlbedo;
layout(location = 3) in vec2 vTexCoord;
layout(location = 4) flat in int vTextureLayer;
layout(location = 5) in float vMetallic;
layout(location = 6) in float vRoughness;
layout(location = 7) in vec4 vEmissive;
layout(location = 8) in vec4 vTangent;

layout(set = 0, binding = 1) uniform sampler2DArray sceneTextures;

#include "scene_ubo.glsl"

#include "clustered_lights.glsl"

#include "color_space.glsl"

#include "ibl.glsl"

layout(push_constant) uniform Push {
    mat4 model;
    vec4 albedoTint;
    int textureLayer;
    int skyMode;
    float metallic;
    float roughness;
    vec4 emissive;
    int useSkinning;
    int jointCount;
    int shadingModel;
    int toonDiffuseBands;
    float toonRimIntensity;
    float toonRimPower;
    int normalMapLayer;
    int metallicRoughnessMapLayer;
    int emissiveMapLayer;
    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    int shadowFlags;
    int pbrPad0;
    vec4 emissiveFactor;
} push;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 1e-6);
}

float G_SchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-6);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float pointAttenuation(float dist, float range) {
    float r = max(range, 0.001);
    float window = clamp(1.0 - dist / r, 0.0, 1.0);
    window *= window;
    float denom = 1.0 + dist * dist;
    return window / max(denom, 1e-4);
}

/** Stable 0–1 hash for rotating PCF offsets (reduces square grid banding vs axis-aligned 3×3). */
float shadowPcfRotation01(vec2 fragPx) {
    return fract(sin(dot(fragPx, vec2(12.9898, 78.233))) * 43758.5453);
}

const float kShadowPcfW5[5] = float[](0.0625, 0.25, 0.375, 0.25, 0.0625);

/**
 * Directional shadow PCF: weighted 5×5 filter + slightly wider kernel when N·L is low (softer grazing penumbra).
 * Offsets are rotated per pixel to reduce axis-aligned blockiness from the shadow map grid.
 */
int selectShadowCascade(float distFromCamera) {
    if (distFromCamera <= ubo.cascadeSplits.x) {
        return 0;
    }
    if (distFromCamera <= ubo.cascadeSplits.y) {
        return 1;
    }
    if (distFromCamera <= ubo.cascadeSplits.z) {
        return 2;
    }
    return 3;
}

vec2 cascadeAtlasUv(vec2 uv01, int cascade) {
    vec4 a = ubo.cascadeAtlas[cascade];
    return uv01 * a.zw + a.xy;
}

float sampleSunShadowPcf(
        sampler2D map,
        vec2 uvCenter,
        float refZ,
        vec2 texel,
        float penumbraScale,
        float rot01) {
    float c = cos(rot01 * 6.28318530718);
    float s = sin(rot01 * 6.28318530718);
    mat2 rot = mat2(c, -s, s, c);
    float acc = 0.0;
    float wsum = 0.0;
    vec2 margin = texel * max(3.0, penumbraScale * 2.5);
    vec2 uvC = clamp(uvCenter, margin, vec2(1.0) - margin);
    for (int j = -2; j <= 2; ++j) {
        for (int i = -2; i <= 2; ++i) {
            float w = kShadowPcfW5[i + 2] * kShadowPcfW5[j + 2];
            vec2 off = rot * (vec2(float(i), float(j)) * texel * penumbraScale);
            vec2 suv = clamp(uvC + off, vec2(0.0), vec2(1.0));
            float mapZ = textureLod(map, suv, 0.0).r;
            acc += w * (refZ <= mapZ ? 1.0 : 0.0);
            wsum += w;
        }
    }
    return acc / max(wsum, 1e-5);
}

/** Depth compare in shader (not sampler2DShadow) for MoltenVK / VK_KHR_portability_subset. */
layout(set = 0, binding = 3) uniform sampler2D shadowMap;

#include "punctual_shadows.glsl"

float sampleSunShadowAtCascade(
        int cascade,
        vec3 worldPos,
        vec3 N,
        vec3 Ld,
        float sunGracing) {
    vec4 ls = ubo.worldToShadowClip[cascade] * vec4(worldPos, 1.0);
    vec3 proj = ls.xyz / max(ls.w, 1e-5);
    vec2 uv = vec2(proj.x * 0.5 + 0.5, proj.y * 0.5 + 0.5);
    uv = cascadeAtlasUv(uv, cascade);
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    float g = sunGracing;
    float depthBias = max(ubo.shadowParams.x, 1e-5) * (1.0 + 6.25 * g);
    depthBias += ubo.shadowParams.y * 0.22 * g;
    float refZ = clamp(proj.z - depthBias, 0.0, 1.0);
    vec2 texel = vec2(ubo.shadowParams.z);
    float penumbraScale = clamp(1.0 + 2.75 * g, 1.0, 2.85);
    float rot01 = shadowPcfRotation01(gl_FragCoord.xy);
    float sh = sampleSunShadowPcf(shadowMap, uv, refZ, texel, penumbraScale, rot01);
    return pow(clamp(sh, 0.0, 1.0), 1.08);
}

/** Soft blend between adjacent CSM cascades near split distances (hides resolution pops). */
float blendSunShadowCascades(float distCam, vec3 worldPos, vec3 N, vec3 Ld, float sunGracing) {
    int cascade = selectShadowCascade(distCam);
    float sh = sampleSunShadowAtCascade(cascade, worldPos, N, Ld, sunGracing);

    float blendFrac = clamp(ubo.timeGlobal.w, 0.001, 0.35);
    const float kMinBlendM = 0.35;

    if (cascade > 0) {
        float splitLow = ubo.cascadeSplits[cascade - 1];
        float blendLen = max(splitLow * blendFrac, kMinBlendM);
        float wPrev = 1.0 - smoothstep(splitLow - blendLen, splitLow, distCam);
        if (wPrev > 1e-4) {
            float shPrev = sampleSunShadowAtCascade(cascade - 1, worldPos, N, Ld, sunGracing);
            sh = mix(sh, shPrev, wPrev);
        }
    }
    if (cascade < 3) {
        float splitHigh = ubo.cascadeSplits[cascade];
        float blendLen = max(splitHigh * blendFrac, kMinBlendM);
        float wNext = smoothstep(splitHigh - blendLen, splitHigh, distCam);
        if (wNext > 1e-4) {
            float shNext = sampleSunShadowAtCascade(cascade + 1, worldPos, N, Ld, sunGracing);
            sh = mix(sh, shNext, wNext);
        }
    }
    return sh;
}

/**
 * Cook-Torrance BRDF. `wrapNL` > 0 softens the diffuse terminator (outdoor sun); use 0 for local lights.
 */
vec3 evalBRDF(
        vec3 N,
        vec3 V,
        vec3 L,
        vec3 baseColor,
        float metallic,
        float roughness,
        vec3 radiance,
        float wrapNL) {
    float rawNL = dot(N, L);
    float NdotL = (wrapNL > 1e-5) ? max((rawNL + wrapNL) / (1.0 + wrapNL), 0.0) : max(rawNL, 0.0);
    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float rough = clamp(roughness, 0.04, 1.0);
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    vec3 F = F_Schlick(VdotH, F0);

    float specNL = max(rawNL, 0.0);
    float D = D_GGX(NdotH, rough);
    float G = G_Smith(NdotV, specNL, rough);
    vec3 specular = (D * G * F) / max(4.0 * NdotV * max(specNL, 1e-4), 1e-6);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * baseColor / PI;

    return (diffuse * NdotL + specular * specNL) * radiance;
}

vec3 evalSpotBrdfFromData(
        vec3 N,
        vec3 V,
        vec3 baseColor,
        float metallic,
        float roughness,
        vec3 lightPos,
        float lightRange,
        vec3 spotDir,
        float cosOuter,
        float cosInner,
        vec3 lightColor,
        float lightIntensity,
        float shadow) {
    vec3 toL = lightPos - vWorldPos;
    float dist = length(toL);
    vec3 L = toL / max(dist, 1e-4);
    float ang = sparkSpotAngularAttenuation(L, spotDir, cosOuter, cosInner);
    if (ang <= 1e-5) {
        return vec3(0.0);
    }
    float att = sparkPointAttenuation(dist, lightRange) * ang;
    vec3 rad = lightColor * lightIntensity * att * shadow;
    return evalBRDF(N, V, L, baseColor, metallic, roughness, rad, 0.0);
}

void accumulatePunctualPbr(vec3 N, vec3 V, vec3 base, float met, float rough, inout vec3 Lo) {
    if (ubo.clusterDepth.w < 0.5) {
        return;
    }
    const bool receiveShadow = (push.shadowFlags & 2) != 0;
    uint numPt = min(clusterLights.numPointLights, kMaxClusteredPointLights);
    for (uint i = 0u; i < numPt; ++i) {
        vec3 toL = clusterLights.pointPositionRange[i].xyz - vWorldPos;
        float dist = length(toL);
        vec3 Lp = toL / max(dist, 1e-4);
        float att = sparkPointAttenuation(dist, clusterLights.pointPositionRange[i].w);
        vec3 rad = clusterLights.pointColorIntensity[i].rgb * clusterLights.pointColorIntensity[i].w * att;
        float sh = 1.0;
        if (receiveShadow && punctualShadow.punctualShadowEnabled > 0u) {
            int pslot = punctualShadow.pointShadowSlotByLight[i];
            if (pslot >= 0) {
                sh = samplePointShadowAtSlot(pslot, vWorldPos, N, Lp);
            }
        }
        Lo += evalBRDF(N, V, Lp, base, met, rough, rad, 0.0) * sh;
    }
    uint numSp = min(clusterLights.numSpotLights, kMaxClusteredSpotLights);
    for (uint si = 0u; si < numSp; ++si) {
        float sh = 1.0;
        if (receiveShadow && punctualShadow.punctualShadowEnabled > 0u) {
            int sslot = punctualShadow.spotShadowSlotByLight[si];
            if (sslot >= 0) {
                vec3 toS = clusterLights.spotPositionRange[si].xyz - vWorldPos;
                vec3 Ls = toS / max(length(toS), 1e-4);
                sh = sampleSpotShadowAtSlot(sslot, vWorldPos, N, Ls);
            }
        }
        Lo += evalSpotBrdfFromData(
                N,
                V,
                base,
                met,
                rough,
                clusterLights.spotPositionRange[si].xyz,
                clusterLights.spotPositionRange[si].w,
                clusterLights.spotDirectionCosOuter[si].xyz,
                clusterLights.spotDirectionCosOuter[si].w,
                clusterLights.spotCosInner[si].x,
                clusterLights.spotColorIntensity[si].rgb,
                clusterLights.spotColorIntensity[si].w,
                sh);
    }
}

void accumulatePunctualToon(
        vec3 N,
        vec3 V,
        vec3 base,
        float met,
        float gloss,
        float b,
        float denom,
        vec3 specTint,
        inout vec3 Lo) {
    if (ubo.clusterDepth.w < 0.5) {
        return;
    }
    const bool receiveShadow = (push.shadowFlags & 2) != 0;
    uint numPt = min(clusterLights.numPointLights, kMaxClusteredPointLights);
    for (uint i = 0u; i < numPt; ++i) {
        vec3 toL = clusterLights.pointPositionRange[i].xyz - vWorldPos;
        float dist = length(toL);
        vec3 Lp = toL / max(dist, 1e-4);
        float att = sparkPointAttenuation(dist, clusterLights.pointPositionRange[i].w);
        vec3 rad = clusterLights.pointColorIntensity[i].rgb * clusterLights.pointColorIntensity[i].w * att;
        float sh = 1.0;
        if (receiveShadow && punctualShadow.punctualShadowEnabled > 0u) {
            int pslot = punctualShadow.pointShadowSlotByLight[i];
            if (pslot >= 0) {
                sh = samplePointShadowAtSlot(pslot, vWorldPos, N, Lp);
            }
        }
        float rawP = dot(N, Lp);
        float ndlp = max(rawP, 0.0);
        float stepP = floor(ndlp * b + 1e-4) / denom;
        Lo += base * (1.0 - met * 0.92) * stepP * rad * sh;
        vec3 Hp = normalize(V + Lp);
        float nhp = max(dot(N, Hp), 0.0);
        float sp = pow(nhp, gloss * 0.85);
        float se = smoothstep(0.28, 0.34, sp);
        Lo += specTint * se * max(rawP, 0.0) * rad * 0.55 * sh;
    }
    uint numSp = min(clusterLights.numSpotLights, kMaxClusteredSpotLights);
    for (uint si = 0u; si < numSp; ++si) {
        vec3 toS = clusterLights.spotPositionRange[si].xyz - vWorldPos;
        float dS = length(toS);
        vec3 Ls = toS / max(dS, 1e-4);
        float cTh = sparkSpotAngularAttenuation(
                Ls,
                clusterLights.spotDirectionCosOuter[si].xyz,
                clusterLights.spotDirectionCosOuter[si].w,
                clusterLights.spotCosInner[si].x);
        float angS = cTh;
        if (angS <= 1e-5) {
            continue;
        }
        float aS = sparkPointAttenuation(dS, clusterLights.spotPositionRange[si].w) * angS;
        vec3 radS = clusterLights.spotColorIntensity[si].rgb * clusterLights.spotColorIntensity[si].w * aS;
        float sh = 1.0;
        if (receiveShadow && punctualShadow.punctualShadowEnabled > 0u) {
            int sslot = punctualShadow.spotShadowSlotByLight[si];
            if (sslot >= 0) {
                sh = sampleSpotShadowAtSlot(sslot, vWorldPos, N, Ls);
            }
        }
        float rawS = dot(N, Ls);
        float nds = max(rawS, 0.0);
        float stepS = floor(nds * b + 1e-4) / denom;
        Lo += base * (1.0 - met * 0.92) * stepS * radS * sh;
        vec3 Hs2 = normalize(V + Ls);
        float nhs = max(dot(N, Hs2), 0.0);
        float sps = pow(nhs, gloss * 0.85);
        float ses = smoothstep(0.28, 0.34, sps);
        Lo += specTint * ses * max(rawS, 0.0) * radS * 0.55 * sh;
    }
}

void main() {
    if (push.skyMode != 0) {
        vec3 cam = ubo.cameraPos.xyz;
        // Per-pixel world view ray — linear interpolation of vWorldPos is inside the sky mesh, not on the
        // sphere/plane, so normalize(interp - cam) warps when the camera rotates (HDR equirect swims).
        vec2 vp = max(ubo.viewportSize.xy, vec2(1.0));
        float sx = (gl_FragCoord.x + 0.5) / vp.x;
        float sy = (gl_FragCoord.y + 0.5) / vp.y;
        // Y: no extra sign flip — negating made equirect appear vertically inverted vs PerspectiveVulkan.
        vec2 ndc = vec2(sx * 2.0 - 1.0, sy * 2.0 - 1.0);
        vec4 farH = ubo.invViewProj * vec4(ndc, 1.0, 1.0);
        vec3 farW = farH.xyz / max(farH.w, 1e-5);
        vec3 dir = normalize(farW - cam);
        vec3 skyCol;
        // Textured sky (box / dome / plane): equirect from view ray. TryLoadFromFile uses
        // stbi_set_flip_vertically_on_load(1), so GPU v=0 is the image file bottom; equirect zenith is
        // usually at the file top → map zenith (mu=0) to v=1 via vEq = 1 - mu.
        if (vTextureLayer >= 0) {
            int layer = clamp(vTextureLayer, 0, 15);
            float phi = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
            phi = fract(phi);
            float mu = acos(clamp(dir.y, -1.0, 1.0)) / PI;
            float vEq = 1.0 - mu;
            vec4 tex = textureLod(sceneTextures, vec3(phi, vEq, float(layer)), 0.0);
            skyCol = sparkSrgbToLinear(tex.rgb) * push.albedoTint.rgb;
        } else if (push.skyMode == 1) {
            vec3 horizon = push.albedoTint.rgb;
            vec3 zenith = mix(horizon, vec3(0.45, 0.62, 0.95), 0.65);
            skyCol = mix(horizon, zenith, clamp(dir.y * 0.5 + 0.5, 0.0, 1.0));
        } else if (push.skyMode == 2) {
            float mu = acos(clamp(dir.y, -1.0, 1.0)) / PI;
            vec3 low = push.albedoTint.rgb;
            vec3 high = vec3(0.55, 0.78, 1.0);
            skyCol = mix(low, high, clamp(1.0 - mu, 0.0, 1.0));
        } else {
            float h = clamp(vTexCoord.y, 0.0, 1.0);
            vec3 bottom = push.albedoTint.rgb;
            vec3 top = vec3(0.75, 0.88, 1.0);
            skyCol = mix(bottom, top, h);
        }
        outColor = vec4(skyCol, 1.0);
        return;
    }

    vec3 base = vAlbedo;
    float alpha = push.albedoTint.a;
    if (vTextureLayer >= 0) {
        int layer = clamp(vTextureLayer, 0, 15);
        vec4 tex = texture(sceneTextures, vec3(vTexCoord, float(layer)));
        base *= sparkSrgbToLinear(tex.rgb);
        alpha *= tex.a;
    }

    float met = clamp(vMetallic * push.metallicFactor, 0.0, 1.0);
    float rough = clamp(vRoughness * push.roughnessFactor, 0.04, 1.0);
    float occlusion = push.occlusionStrength;
    if (push.metallicRoughnessMapLayer >= 0) {
        int mrl = clamp(push.metallicRoughnessMapLayer, 0, 15);
        vec3 orm = texture(sceneTextures, vec3(vTexCoord, float(mrl))).rgb;
        rough = clamp(orm.g * push.roughnessFactor, 0.04, 1.0);
        met = clamp(orm.b * push.metallicFactor, 0.0, 1.0);
        occlusion = push.occlusionStrength > 0.0
            ? clamp(orm.r * push.occlusionStrength, 0.08, 1.0)
            : 0.0;
    }

    vec3 nGeom = normalize(vNormal);
    vec3 N = nGeom;
    if (push.normalMapLayer >= 0) {
        int nl = clamp(push.normalMapLayer, 0, 15);
        vec3 tN = texture(sceneTextures, vec3(vTexCoord, float(nl))).xyz * 2.0 - 1.0;
        tN.y = -tN.y;
        mat3 TBN;
        if (dot(vTangent.xyz, vTangent.xyz) > 1e-8) {
            vec3 T = normalize(vTangent.xyz);
            vec3 B = normalize(cross(nGeom, T) * vTangent.w);
            TBN = mat3(T, B, nGeom);
        } else {
            vec3 dp1 = dFdx(vWorldPos);
            vec3 dp2 = dFdy(vWorldPos);
            vec2 duv1 = dFdx(vTexCoord);
            vec2 duv2 = dFdy(vTexCoord);
            float det = duv1.x * duv2.y - duv1.y * duv2.x;
            if (abs(det) > 1e-8) {
                float invDet = 1.0 / det;
                vec3 Tu = (dp1 * duv2.y - dp2 * duv1.y) * invDet;
                vec3 Tv = (dp2 * duv1.x - dp1 * duv2.x) * invDet;
                vec3 T = normalize(Tu - nGeom * dot(nGeom, Tu));
                float handedness = (det < 0.0) ? -1.0 : 1.0;
                vec3 B = normalize(cross(nGeom, T) * handedness);
                TBN = mat3(T, B, nGeom);
            } else {
                TBN = mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), nGeom);
            }
        }
        N = normalize(TBN * tN);
    }

    vec3 V = normalize(ubo.cameraPos.xyz - vWorldPos);

    vec3 Lo = vec3(0.0);

    vec3 Ld = normalize(ubo.lightDir.xyz);
    vec3 sunRad = ubo.lightColor.rgb * ubo.lightColor.w;
    vec3 ambGround = ubo.ambientColor.rgb;
    vec3 ambSkyBase = ubo.ambientSky.rgb;
    const float sunDiffuseWrap = 0.22;
    float sunGracing = 1.0 - clamp(dot(N, Ld), 0.0, 1.0);

    float sunShadow = 1.0;
    if (push.skyMode == 0 && ubo.shadowParams.w > 0.5 && (push.shadowFlags & 2) != 0) {
        float distCam = length(vWorldPos - ubo.cameraPos.xyz);
        sunShadow = blendSunShadowCascades(distCam, vWorldPos, N, Ld, sunGracing);
        float shadowFadeEnd = ubo.viewportSize.z;
        if (shadowFadeEnd > 0.5) {
            float fadeStart = shadowFadeEnd * clamp(ubo.timeGlobal.y, 0.5, 0.98);
            float fade = smoothstep(fadeStart, shadowFadeEnd, distCam);
            sunShadow = mix(sunShadow, 1.0, fade);
        }
    }

    /** Toon/cel: banded wrap diffuse, hard spec band, view rim (push.toon*). */
    if (push.shadingModel == 1) {
        float b = clamp(float(push.toonDiffuseBands), 2.0, 8.0);
        float denom = max(b - 1.0, 1.0);
        float rawNL = dot(N, Ld);
        float ndl = max((rawNL + sunDiffuseWrap) / (1.0 + sunDiffuseWrap), 0.0);
        float steppedNL = floor(ndl * b + 1e-4) / denom;

        vec3 sunDiffuse = base * (1.0 - met * 0.92) * steppedNL * sunRad * sunShadow;

        vec3 Hs = normalize(V + Ld);
        float nh = max(dot(N, Hs), 0.0);
        float gloss = mix(120.0, 14.0, rough);
        float specTerm = pow(nh, gloss);
        float specEdge = smoothstep(0.22, 0.30, specTerm);
        vec3 specTint = mix(vec3(0.06), vec3(1.0), met);
        vec3 specCol = specTint * specEdge * max(rawNL, 0.0) * sunRad * sunShadow * 0.7;

        float nv = clamp(dot(N, V), 0.0, 1.0);
        float rimAmt = pow(1.0 - nv, max(push.toonRimPower, 0.35));
        vec3 rimCol =
                rimAmt * clamp(push.toonRimIntensity, 0.0, 4.0) * mix(vec3(0.32, 0.48, 1.0), sunRad, 0.45) * steppedNL;

        Lo = sunDiffuse + specCol + rimCol;

        float skyFillT = (1.0 - sunShadow) * (0.08 + 0.12 * sunGracing);
        vec3 skyHueT = mix(ambGround, ambSkyBase, 0.72);
        float ndUpT = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0);
        Lo += base * (1.0 - met * 0.85) * ndUpT * skyHueT * skyFillT * max(length(sunRad), 1e-4);

        accumulatePunctualToon(N, V, base, met, gloss, b, denom, specTint, Lo);

        float hemiT = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 ambTintT = mix(ambGround, ambSkyBase, hemiT);
        vec3 ambientT = ambTintT * base * 0.28 * (1.0 - met * 0.4);
        ambientT += ubo.ambientProbe.rgb * ubo.ambientProbe.w * base * 0.22 * (1.0 - met * 0.35);
        float aoT = mix(0.78, 1.0, pow(clamp(dot(N, V), 0.0, 1.0), 0.55));
        ambientT *= aoT * occlusion;

        vec3 emissiveT = vEmissive.rgb * vEmissive.w * push.emissiveFactor.rgb;
        if (push.emissiveMapLayer >= 0) {
            int el = clamp(push.emissiveMapLayer, 0, 15);
            emissiveT *= texture(sceneTextures, vec3(vTexCoord, float(el))).rgb;
        }
        outColor = vec4(ambientT + Lo + emissiveT, alpha);
        return;
    }

    vec3 sunBrdf = evalBRDF(N, V, Ld, base, met, rough, sunRad, sunDiffuseWrap);
    Lo += sunBrdf * sunShadow;
    // Skylight bounce in sun shadow / penumbra (diffuse-only; cool tint vs black terminator).
    float skyFill = (1.0 - sunShadow) * (0.11 + 0.14 * sunGracing);
    vec3 skyHue = mix(ambGround, ambSkyBase, 0.72);
    float ndUp = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0);
    Lo += base * (1.0 - met) * (1.0 / PI) * ndUp * skyHue * skyFill * max(length(sunRad), 1e-4);

    accumulatePunctualPbr(N, V, base, met, rough, Lo);

    float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambTint = mix(ambGround, ambSkyBase, hemi);
    vec3 ambient = ambTint * base * mix(1.0, 0.35, met);
    ambient += ubo.ambientProbe.rgb * ubo.ambientProbe.w * base * mix(0.35, 0.12, met);
    float aoAmb = mix(0.74, 1.0, pow(clamp(dot(N, V), 0.0, 1.0), 0.58));
    ambient *= aoAmb * occlusion;

    vec3 iblSpecular = sparkEvalSpecularIbl(sceneTextures, N, V, base, met, rough, occlusion);
    vec3 iblDiffuse = sparkEvalDiffuseIbl(sceneTextures, N, base, met, aoAmb * occlusion);
    if (ubo.iblParams.w > 0.5) {
        ambient = mix(ambient, iblDiffuse, mix(0.55, 0.92, met));
        ambient *= mix(1.0, 0.12, met);
    }

    vec3 emissive = vEmissive.rgb * vEmissive.w * push.emissiveFactor.rgb;
    if (push.emissiveMapLayer >= 0) {
        int el = clamp(push.emissiveMapLayer, 0, 15);
        emissive *= texture(sceneTextures, vec3(vTexCoord, float(el))).rgb;
    }

    outColor = vec4(ambient + Lo + iblSpecular + emissive, alpha);
}
