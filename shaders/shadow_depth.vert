#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inJointsPacked;
layout(location = 5) in vec4 inWeights;

layout(std430, set = 0, binding = 2) readonly buffer SkinSSBO {
    mat4 bones[];
} skin;

layout(push_constant) uniform ShadowPush {
    mat4 model;
    mat4 lightViewProj;
    int useSkinning;
    int jointCount;
    int _sp0;
    int _sp1;
} sp;

void main() {
    vec3 worldPos;
    uvec4 inJoints = uvec4(
            floatBitsToUint(inJointsPacked.x),
            floatBitsToUint(inJointsPacked.y),
            floatBitsToUint(inJointsPacked.z),
            floatBitsToUint(inJointsPacked.w));

    if (sp.useSkinning != 0) {
        vec4 spv = vec4(0.0);
        for (int k = 0; k < 4; k++) {
            float w = inWeights[k];
            if (w <= 0.0) {
                continue;
            }
            uint ji = inJoints[k];
            if (int(ji) >= sp.jointCount) {
                continue;
            }
            mat4 BM = skin.bones[ji];
            spv += w * BM * vec4(inPosition, 1.0);
        }
        vec4 wp = sp.model * vec4(spv.xyz, 1.0);
        worldPos = wp.xyz;
    } else {
        vec4 wp = sp.model * vec4(inPosition, 1.0);
        worldPos = wp.xyz;
    }

    gl_Position = sp.lightViewProj * vec4(worldPos, 1.0);
}
