#version 450

layout(set = 0, binding = 0) uniform sampler2DArray uiAtlas;

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float vAtlasLayer;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(uiAtlas, vec3(vUv, vAtlasLayer));
    outColor = vec4(tex.rgb * vColor.rgb, tex.a * vColor.a);
}
