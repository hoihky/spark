#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform UiPush {
    vec4 screenSize;
} up;

layout(location = 0) out vec4 vColor;

void main() {
    vColor = inColor;
    vec2 sz = max(up.screenSize.xy, vec2(1.0));
    float ndcX = inPos.x / sz.x * 2.0 - 1.0;
    float ndcY = inPos.y / sz.y * 2.0 - 1.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
}
