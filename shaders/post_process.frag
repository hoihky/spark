#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 outColor;

#include "post_common.glsl"

const vec2 kKernel[16] = vec2[](
    vec2(0.05, 0.02), vec2(-0.04, 0.06), vec2(0.07, -0.05), vec2(-0.06, -0.03),
    vec2(0.02, 0.09), vec2(-0.08, 0.01), vec2(0.09, 0.07), vec2(-0.03, -0.08),
    vec2(0.06, -0.07), vec2(-0.07, 0.08), vec2(0.01, -0.09), vec2(-0.09, -0.02),
    vec2(0.08, 0.04), vec2(-0.02, 0.10), vec2(0.10, -0.01), vec2(-0.10, -0.06));

float clipDepth(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    return clip.z / max(clip.w, 1e-5);
}

float computeSsao(vec2 uv, vec3 worldPos, vec3 worldNormal) {
    if (post.ssaoEnabled < 0.5) {
        return 1.0;
    }
    vec3 cam = ubo.cameraPos.xyz;
    vec3 viewDir = normalize(cam - worldPos);
    vec3 tangent = normalize(cross(worldNormal, viewDir));
    if (dot(tangent, tangent) < 1e-4) {
        tangent = normalize(cross(worldNormal, vec3(0.0, 1.0, 0.0)));
    }
    vec3 bitangent = cross(worldNormal, tangent);

    float occlusion = 0.0;
    float radius = max(post.ssaoRadius, 1e-3);
    for (int i = 0; i < 16; ++i) {
        vec3 offset = tangent * kKernel[i].x + bitangent * kKernel[i].y;
        offset *= radius;
        vec3 samplePos = worldPos + offset;
        vec2 sampleUv = projectWorldToScreenUv(samplePos);
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 || sampleUv.y < 0.0 || sampleUv.y > 1.0) {
            continue;
        }
        float sampleDepth = sampleSceneDepth(sampleUv);
        vec3 sampleWorld = reconstructWorldPos(sampleUv, sampleDepth);
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(length(cam - sampleWorld), 1e-3));
        float expectedZ = clipDepth(samplePos);
        float sampleZ = clipDepth(sampleWorld);
        float occluded = (sampleZ >= expectedZ + post.ssaoBias) ? 1.0 : 0.0;
        occlusion += occluded * rangeCheck;
    }
    float ao = 1.0 - clamp(occlusion / 16.0, 0.0, 1.0);
    return mix(1.0, ao, clamp(post.ssaoStrength, 0.0, 1.0));
}

void main() {
    vec2 uv = vUv;
    vec3 hdr = texture(sceneColorTex, uv).rgb;
    float depth = sampleSceneDepth(uv);
    if (depth >= 0.9999) {
        outColor = vec4(hdr, 1.0);
        return;
    }

    vec2 texelSize = vec2(1.0) / max(ubo.viewportSize.xy, vec2(1.0));
    vec3 worldPos = reconstructWorldPos(uv, depth);
    vec3 worldNormal = viewNormalFromDepth(uv, texelSize);
    float ao = computeSsao(uv, worldPos, worldNormal);

    outColor = vec4(hdr * ao, 1.0);
}
