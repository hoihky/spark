#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

#include "foliage_instance.glsl"
#include "wind.glsl"

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTex;
layout(location = 3) flat out int vTextureLayer;
layout(location = 4) out vec4 vTint;
layout(location = 5) out float vHeight01;

void main() {
    FoliageInstanceGpu inst = foliageInstances.instances[foliageBatch.instanceBase + uint(gl_InstanceIndex)];
    vec3 localPos = sparkWindBendLocalPosition(
            inPosition, foliageBatch.bladeHeight, foliageBatch.windBendScale, inst.windPhase);
    vec4 worldPos4 = inst.model * vec4(localPos, 1.0);
    vec3 worldPos = worldPos4.xyz;
    gl_Position = ubo.viewProj * worldPos4;
    vWorldPos = worldPos;
    vNormal = normalize(mat3(inst.model) * inNormal);
    vTex = inTexCoord;
    vTextureLayer = foliageBatch.textureLayer;
    vTint = inst.tint * foliageBatch.albedoTint;
    vHeight01 = clamp(inPosition.y / max(foliageBatch.bladeHeight, 1e-4), 0.0, 1.0);
}
