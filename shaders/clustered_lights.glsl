// Shared clustered forward lighting (included by scene.frag / sprite.frag).
#ifndef SPARK_CLUSTERED_LIGHTS_GLSL
#define SPARK_CLUSTERED_LIGHTS_GLSL

const uint kMaxClusteredPointLights = 256u;
const uint kMaxClusteredSpotLights = 128u;

layout(std430, set = 0, binding = 4) readonly buffer ClusterLightsSSBO {
    uint numPointLights;
    uint numSpotLights;
    uint _lightsPad0;
    uint _lightsPad1;
    vec4 pointPositionRange[256];
    vec4 pointColorIntensity[256];
    vec4 spotPositionRange[128];
    vec4 spotDirectionCosOuter[128];
    vec4 spotColorIntensity[128];
    vec4 spotCosInner[128];
} clusterLights;

// Cluster grid kept for future tiled path; punctual shading reads lights SSBO directly.
layout(std430, set = 0, binding = 5) readonly buffer ClusterGridSSBO {
    uint clusterOffsets[256];
    uint clusterCounts[256];
    uint clusterIndices[16384];
} clusterGrid;

float sparkPointAttenuation(float dist, float range) {
    float att = max(1.0 - dist / max(range, 1e-3), 0.0);
    return att * att;
}

/** @p toLightDir fragment → light; @p spotEmissionDir world direction the spot shines (away from the fixture). */
float sparkSpotAngularAttenuation(vec3 toLightDir, vec3 spotEmissionDir, float cosOuter, float cosInner) {
    float cosTheta = dot(normalize(toLightDir), normalize(-spotEmissionDir));
    return smoothstep(cosOuter, cosInner, cosTheta);
}

#endif
