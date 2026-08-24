#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/scene/mesh/Mesh.hpp"

struct cgltf_data;
struct cgltf_mesh;
struct cgltf_primitive;

namespace Spark {

/**
 * Builds CPU <c>Mesh</c> geometry from glTF primitives (local or transformed space).
 * Shared by merged <c>GltfRigidLoader</c> and per-mesh <c>GltfSceneLoader</c>.
 */
class GltfMeshBuilder {
public:
    /** Appends one triangle primitive into <c>outMesh</c>, transforming attributes by <c>transform</c>. */
    static void AppendPrimitive(
            const cgltf_data* data,
            const cgltf_primitive* prim,
            const Matrix4& transform,
            Mesh& outMesh);

    /** Builds a mesh from a glTF mesh in local space (identity transform per primitive). */
    [[nodiscard]] static bool BuildFromCgltfMesh(const cgltf_data* data, const cgltf_mesh* mesh, Mesh& outMesh);
};

}  // namespace Spark
