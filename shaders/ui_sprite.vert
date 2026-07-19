#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inUvLayer;
layout(location = 2) in vec4 inColor;

layout(push_constant) uniform UiSpritePush {
    vec4 screenSize;
} sp;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
layout(location = 2) out float vAtlasLayer;

void main() {
    vUv = inUvLayer.xy;
    vAtlasLayer = inUvLayer.z;
    vColor = inColor;
    vec2 sz = max(sp.screenSize.xy, vec2(1.0));
    float ndcX = inPos.x / sz.x * 2.0 - 1.0;
    float ndcY = inPos.y / sz.y * 2.0 - 1.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
}
