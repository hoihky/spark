#version 450

layout(set = 0, binding = 0) uniform sampler2DArray fontAtlas;

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float vAtlasLayer;

layout(location = 0) out vec4 outColor;

void main() {
    float a = texture(fontAtlas, vec3(vUv, vAtlasLayer)).a;
    outColor = vec4(vColor.rgb, vColor.a * a);
}
