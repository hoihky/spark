#version 450

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vDisc;
layout(location = 2) in vec2 vUv;
layout(location = 3) flat in float vLayer;

layout(set = 1, binding = 10) uniform sampler2DArray particleSceneTextures;

layout(location = 0) out vec4 outColor;

void main() {
    float d = length(vDisc);
    float edge = 1.0 - smoothstep(0.82, 1.02, d);
    if (edge < 0.004) {
        discard;
    }

    vec4 col = vColor;
    if (vLayer >= 0.0) {
        vec4 tex = textureLod(particleSceneTextures, vec3(vUv, vLayer), 0.0);
        col.rgb *= tex.rgb;
        col.a *= tex.a;
    }

    outColor = vec4(col.rgb, col.a * edge);
}
