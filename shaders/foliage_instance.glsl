#ifndef SPARK_FOLIAGE_INSTANCE_GLSL
#define SPARK_FOLIAGE_INSTANCE_GLSL

struct FoliageInstanceGpu {
    mat4 model;
    vec4 tint;
    float windPhase;
    float pad0;
    float pad1;
    float pad2;
};

layout(std430, set = 0, binding = 15) readonly buffer FoliageInstanceBuffer {
    FoliageInstanceGpu instances[];
} foliageInstances;

layout(push_constant) uniform FoliageBatchPush {
    uint instanceBase;
    int textureLayer;
    float alphaCutoff;
    float bladeHeight;
    float windBendScale;
    float padding0;
    float padding1;
    vec4 albedoTint;
} foliageBatch;

#endif
