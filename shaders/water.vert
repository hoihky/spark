#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

#include "scene_ubo.glsl"
#include "water_push.glsl"
#include "gerstner_wave.glsl"

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vWorldXZ;
layout(location = 2) out vec2 vScreenUv;
layout(location = 3) out vec4 vBaseColor;

void main() {
    vec2 worldXZ = waterPush.tileAnchorXZ + inPosition.xz;
    vec3 displacement =
            sparkGerstnerDisplacement(worldXZ, waterPush.timeSeconds, waterPush.waveCount, waterPush.waves);
    vec3 worldPos = vec3(
            worldXZ.x + displacement.x,
            waterPush.waterLevelY + displacement.y,
            worldXZ.y + displacement.z);

    vWorldPos = worldPos;
    vWorldXZ = worldXZ;
    vBaseColor = waterPush.baseColor;

    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    float zw = max(clip.w, 1.0e-5);
    clip.z = min(clip.z, zw * (1.0 - 2.0e-4));
    gl_Position = clip;
    vec2 ndc = clip.xy / zw;
    vScreenUv = ndc * 0.5 + 0.5;
}
