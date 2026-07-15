#version 450

layout(location = 0) in vec2 vUv;

layout(set = 0, binding = 0) uniform sampler2D hdrColor;

layout(push_constant) uniform TonemapPush {
    float exposure;
    float invGamma;
    float pad0;
    float pad1;
} push;

layout(location = 0) out vec4 outColor;

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(hdrColor, vUv).rgb * max(push.exposure, 1e-4);
    vec3 ldr = acesTonemap(hdr);
    ldr = pow(ldr, vec3(max(push.invGamma, 1e-3)));
    outColor = vec4(ldr, 1.0);
}
