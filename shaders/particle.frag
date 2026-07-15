#version 450

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vDisc;

layout(location = 0) out vec4 outColor;

void main() {
    float d = length(vDisc);
    float edge = 1.0 - smoothstep(0.82, 1.02, d);
    if (edge < 0.004) {
        discard;
    }
    outColor = vec4(vColor.rgb, vColor.a * edge);
}
