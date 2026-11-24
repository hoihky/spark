#ifndef SPARK_MESH_INSTANCE_GLSL
#define SPARK_MESH_INSTANCE_GLSL

struct RigidMeshInstanceGpu {
    mat4 model;
};

layout(std430, set = 0, binding = 16) readonly buffer RigidMeshInstanceBuffer {
    RigidMeshInstanceGpu instances[];
} rigidMeshInstances;

#endif