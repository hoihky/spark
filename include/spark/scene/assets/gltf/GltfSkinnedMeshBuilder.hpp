#pragma once

#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

struct cgltf_data;
struct cgltf_mesh;
struct cgltf_primitive;

namespace Spark {

/**
 * Builds CPU <c>SkinnedMesh</c> geometry via <c>GltfPrimitiveDecoderRegistry</c>
 * (uncompressed, Draco, and meshopt-decompressed accessors).
 */
class GltfSkinnedMeshBuilder {
public:
    [[nodiscard]] static GltfMeshBuildOutcome AppendSkinnedPrimitive(
            const cgltf_data* data,
            const cgltf_primitive* prim,
            const Matrix4& bakeWorld,
            std::uint32_t texCoordSet,
            bool* outHadNormals,
            bool* outHadTangents,
            SkinnedMesh& outMesh);

    [[nodiscard]] static GltfMeshBuildOutcome BuildFromCgltfMesh(
            const cgltf_data* data,
            const cgltf_mesh* mesh,
            const Matrix4& bakeWorld,
            std::uint32_t texCoordSet,
            bool* outHadNormals,
            bool* outHadTangents,
            SkinnedMesh& outMesh);
};

}  // namespace Spark
