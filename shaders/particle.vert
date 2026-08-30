#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;
layout(location = 3) in vec2 inCorner;
layout(location = 4) in vec2 inUv;
layout(location = 5) in float inLayer;

layout(std140, set = 0, binding = 0) uniform ParticleUBO {
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraRight;
    vec4 cameraUp;
};

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vDisc;
layout(location = 2) out vec2 vUv;
layout(location = 3) flat out float vLayer;

void main() {
    vec3 right = normalize(cameraRight.xyz);
    vec3 up = normalize(cameraUp.xyz);
    vec3 worldPos = inPos + right * inCorner.x * inSize * 0.5 + up * inCorner.y * inSize * 0.5;
    gl_Position = viewProj * vec4(worldPos, 1.0);
    vColor = inColor;
    vDisc = inCorner;
    vUv = inUv;
    vLayer = inLayer;
}
