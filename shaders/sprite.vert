#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

#include "scene_ubo.glsl"
#include "sprite_instance.glsl"

layout(location = 0) out vec2 vTex;
layout(location = 1) flat out int vLayer;
layout(location = 2) out vec2 vLocalXY;
layout(location = 3) out vec3 vWorldPos;
layout(location = 4) flat out vec4 vTint;
layout(location = 5) flat out int vLightingMode;
layout(location = 6) flat out vec4 vLightingA;
layout(location = 7) flat out vec4 vLightingB;

void main() {
    SpriteInstanceGpu inst = instances[spriteBatch.instanceBase + uint(gl_InstanceIndex)];
    vec4 wp = inst.model * vec4(inPosition, 1.0);
    gl_Position = ubo.viewProj * wp;
    vec2 uv01 = inTexCoord;
    const float kLayerSize = 512.0;
    vec2 halfTexel = vec2(0.5) / vec2(kLayerSize);
    vec2 uvMin = inst.uvRect.xy + halfTexel;
    vec2 uvMax = inst.uvRect.zw - halfTexel;
    vTex = mix(uvMin, uvMax, uv01);
    vLayer = inst.textureLayer;
    vLocalXY = inPosition.xy;
    vWorldPos = wp.xyz;
    vTint = inst.tint;
    vLightingMode = inst.lightingMode;
    vLightingA = inst.lightingA;
    vLightingB = inst.lightingB;
}
