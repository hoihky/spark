#version 450

layout(location = 0) in vec2 inPos;
// Pack atlas array layer with UV so the vertex descriptor stays at 3 attributes (MoltenVK is strict
// about float-only location 3 vs the MTLVertexDescriptor on some OS/driver combos).
layout(location = 1) in vec3 inUvLayer;
layout(location = 2) in vec4 inColor;

// Single vec4 avoids driver/layout quirks with multiple vec2 push constants (16 bytes total).
layout(push_constant) uniform TextPush {
    vec4 screenSize;
} tp;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
layout(location = 2) out float vAtlasLayer;

void main() {
    vUv = inUvLayer.xy;
    vAtlasLayer = inUvLayer.z;
    vColor = inColor;
    vec2 sz = max(tp.screenSize.xy, vec2(1.0));
    vec2 ndc;
    ndc.x = inPos.x / sz.x * 2.0 - 1.0;
    ndc.y = inPos.y / sz.y * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
