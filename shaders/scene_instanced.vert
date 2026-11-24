#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inJointsPacked;
layout(location = 5) in vec4 inWeights;
layout(location = 6) in vec2 inTexCoord1;
layout(location = 7) in vec4 inColor;

#include "scene_ubo.glsl"
#include "scene_push.glsl"
#include "mesh_instance.glsl"

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vAlbedo;
layout(location = 3) out vec2 vTexCoord0;
layout(location = 4) flat out int vTextureLayer;
layout(location = 5) out float vMetallic;
layout(location = 6) out float vRoughness;
layout(location = 7) out vec4 vEmissive;
layout(location = 8) out vec4 vTangent;
layout(location = 9) out vec2 vTexCoord1;
layout(location = 10) out vec4 vVertexColor;

void main() {
    mat4 instanceModel =
            rigidMeshInstances.instances[push.instanceBase + uint(gl_InstanceIndex)].model;

    vec4 wp = instanceModel * vec4(inPosition, 1.0);
    vec3 worldPos = wp.xyz;
    mat3 R = mat3(instanceModel);
    float detR = determinant(R);
    mat3 nMat = (abs(detR) > 1e-8) ? transpose(inverse(R)) : mat3(1.0);
    vec3 nW = nMat * inNormal;
    float nLen2 = dot(nW, nW);
    vec3 worldNrm = (nLen2 > 1e-10) ? (nW * inversesqrt(nLen2)) : vec3(0.0, 1.0, 0.0);
    vec3 worldTan = vec3(0.0);
    if (dot(inTangent.xyz, inTangent.xyz) > 1e-8) {
        vec3 tW = R * inTangent.xyz;
        float tLen2 = dot(tW, tW);
        worldTan = (tLen2 > 1e-10) ? (tW * inversesqrt(tLen2)) : vec3(0.0);
    }

    vWorldPos = worldPos;
    vNormal = worldNrm;
    vAlbedo = push.albedoTint.rgb;
    vTexCoord0 = inTexCoord;
    vTexCoord1 = inTexCoord1;
    vVertexColor = inColor;
    vTextureLayer = push.textureLayer;
    vMetallic = push.metallic;
    vRoughness = push.roughness;
    vEmissive = push.emissive;
    vTangent = vec4(worldTan, inTangent.w);
  gl_Position = ubo.viewProj * vec4(worldPos, 1.0);
}