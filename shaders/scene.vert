#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;
// Packed joint indices (bit pattern of uint stored in float) — avoids integer vertex fetch issues on some MVK paths.
layout(location = 4) in vec4 inJointsPacked;
layout(location = 5) in vec4 inWeights;

#include "scene_ubo.glsl"

layout(std430, set = 0, binding = 2) readonly buffer SkinSSBO {
    mat4 bones[];
} skin;

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

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vAlbedo;
layout(location = 3) out vec2 vTexCoord;
layout(location = 4) flat out int vTextureLayer;
layout(location = 5) out float vMetallic;
layout(location = 6) out float vRoughness;
layout(location = 7) out vec4 vEmissive;
layout(location = 8) out vec4 vTangent;

void main() {
    vec3 worldPos;
    vec3 worldNrm;
    vec3 worldTan = vec3(0.0);

    uvec4 inJoints = uvec4(
            floatBitsToUint(inJointsPacked.x),
            floatBitsToUint(inJointsPacked.y),
            floatBitsToUint(inJointsPacked.z),
            floatBitsToUint(inJointsPacked.w));

    if (push.useSkinning != 0) {
        vec4 sp = vec4(0.0);
        mat3 acc = mat3(0.0);
        for (int k = 0; k < 4; k++) {
            float w = inWeights[k];
            if (w <= 0.0) {
                continue;
            }
            uint ji = inJoints[k];
            if (int(ji) >= push.jointCount) {
                continue;
            }
            mat4 BM = skin.bones[ji];
            sp += w * BM * vec4(inPosition, 1.0);
            acc += w * mat3(BM);
        }
        vec3 nLin = acc * inNormal;
        worldNrm = length(nLin) > 1e-5 ? normalize(nLin) : normalize(inNormal);
        if (dot(inTangent.xyz, inTangent.xyz) > 1e-8) {
            vec3 tLin = acc * inTangent.xyz;
            worldTan = length(tLin) > 1e-5 ? normalize(tLin) : vec3(0.0);
        }
        vec4 wp = push.model * vec4(sp.xyz, 1.0);
        worldPos = wp.xyz;
        mat3 R = mat3(push.model);
        float detR = determinant(R);
        mat3 nMat = (abs(detR) > 1e-8) ? transpose(inverse(R)) : mat3(1.0);
        vec3 nW = nMat * worldNrm;
        float nLen2 = dot(nW, nW);
        vNormal = (nLen2 > 1e-10) ? (nW * inversesqrt(nLen2)) : vec3(0.0, 1.0, 0.0);
        if (dot(worldTan, worldTan) > 1e-8) {
            vec3 tW = nMat * worldTan;
            float tLen2 = dot(tW, tW);
            worldTan = (tLen2 > 1e-10) ? (tW * inversesqrt(tLen2)) : vec3(0.0);
        }
    } else {
        vec4 wp = push.model * vec4(inPosition, 1.0);
        worldPos = wp.xyz;
        mat3 R = mat3(push.model);
        float detR = determinant(R);
        mat3 nMat = (abs(detR) > 1e-8) ? transpose(inverse(R)) : mat3(1.0);
        vec3 nW = nMat * inNormal;
        float nLen2 = dot(nW, nW);
        vNormal = (nLen2 > 1e-10) ? (nW * inversesqrt(nLen2)) : vec3(0.0, 1.0, 0.0);
        if (dot(inTangent.xyz, inTangent.xyz) > 1e-8) {
            vec3 tW = R * inTangent.xyz;
            float tLen2 = dot(tW, tW);
            worldTan = (tLen2 > 1e-10) ? (tW * inversesqrt(tLen2)) : vec3(0.0);
        }
    }

    vWorldPos = worldPos;
    vAlbedo = push.albedoTint.rgb;
    vTexCoord = inTexCoord;
    vTextureLayer = push.textureLayer;
    vMetallic = push.metallic;
    vRoughness = push.roughness;
    vEmissive = push.emissive;
    vTangent = vec4(worldTan, inTangent.w);
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    if (push.skyMode != 0) {
        // Push clip z slightly below w so NDC depth < 1 (depth buffer clears to 1; pipeline skips depth test).
        float zw = max(clip.w, 1e-5);
        gl_Position = vec4(clip.x, clip.y, zw * (1.0 - 1e-4), zw);
    } else {
        gl_Position = clip;
    }
}
