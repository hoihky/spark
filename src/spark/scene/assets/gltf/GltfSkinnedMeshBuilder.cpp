#include "spark/scene/assets/gltf/GltfSkinnedMeshBuilder.hpp"

#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

bool ShouldFlipGltfTriangleWinding(const Matrix4& bakeWorld) noexcept {
    return bakeWorld.DeterminantUpper3x3() >= 0.0F;
}

std::uint32_t MaterialIndex(const cgltf_data* data, const cgltf_material* mat) noexcept {
    if (data == nullptr || mat == nullptr || data->materials_count == 0) {
        return 0;
    }
    const std::ptrdiff_t offset = mat - data->materials;
    if (offset >= 0 && static_cast<cgltf_size>(offset) < data->materials_count) {
        return static_cast<std::uint32_t>(offset);
    }
    return 0;
}

}  // namespace

GltfMeshBuildOutcome GltfSkinnedMeshBuilder::AppendSkinnedPrimitive(
        const cgltf_data* data,
        const cgltf_primitive* prim,
        const Matrix4& bakeWorld,
        const std::uint32_t texCoordSet,
        bool* outHadNormals,
        bool* outHadTangents,
        SkinnedMesh& outMesh) {
    GltfMeshBuildOutcome outcome{};
    if (prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Unsupported skinned glTF primitive");
        return outcome;
    }

    GltfPrimitiveDecodeOptions options{};
    options.texCoordSet = texCoordSet;
    const GltfPrimitiveDecodeResult decoded = GltfPrimitiveDecoderRegistry::Decode(data, prim, options);
    if (!decoded.ok) {
        outcome.ok = false;
        outcome.errorMessage = decoded.errorMessage;
        return outcome;
    }
    if (!decoded.primitive.HasSkinning()) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Skinned glTF primitive is missing JOINTS_0 / WEIGHTS_0");
        return outcome;
    }
    if (decoded.primitive.positions.IsEmpty()) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Decoded skinned glTF primitive has no vertices");
        return outcome;
    }

    const std::uint32_t indexOffset = static_cast<std::uint32_t>(outMesh.GetIndices().GetSize());
    if (outHadNormals != nullptr) {
        *outHadNormals = *outHadNormals || decoded.primitive.normals.GetSize() == decoded.primitive.positions.GetSize();
    }
    if (outHadTangents != nullptr) {
        *outHadTangents = *outHadTangents || decoded.primitive.tangents.GetSize() == decoded.primitive.positions.GetSize();
    }

    const std::uint32_t base = static_cast<std::uint32_t>(outMesh.GetVertices().GetSize());
    const bool flipWinding = ShouldFlipGltfTriangleWinding(bakeWorld);
    const cgltf_size vertexCount = decoded.primitive.positions.GetSize();
    const bool hasNormals = decoded.primitive.normals.GetSize() == vertexCount;
    const bool hasTexcoords = decoded.primitive.texcoords.GetSize() == vertexCount;
    const bool hasTexcoords1 = decoded.primitive.texcoords1.GetSize() == vertexCount;
    const bool hasColors = decoded.primitive.colors.GetSize() == vertexCount;
    const bool hasTangents = decoded.primitive.tangents.GetSize() == vertexCount;

    for (cgltf_size vi = 0; vi < vertexCount; ++vi) {
        const Vector3 pw = bakeWorld.TransformPoint(decoded.primitive.positions[vi]);
        Vector3 nw{0.0F, 1.0F, 0.0F};
        if (hasNormals) {
            nw = bakeWorld.TransformVector(decoded.primitive.normals[vi]).Normalized();
        }
        Vector2 tc{0.0F, 0.0F};
        if (hasTexcoords) {
            tc = decoded.primitive.texcoords[vi];
        }
        Vector2 tc1{0.0F, 0.0F};
        if (hasTexcoords1) {
            tc1 = decoded.primitive.texcoords1[vi];
        }
        Vector4 color{1.0F, 1.0F, 1.0F, 1.0F};
        if (hasColors) {
            color = decoded.primitive.colors[vi];
        }
        Vector4 tangent{};
        if (hasTangents) {
            const Vector4& localTangent = decoded.primitive.tangents[vi];
            const Vector3 tw = bakeWorld.TransformVector({localTangent.x, localTangent.y, localTangent.z}).Normalized();
            tangent = {tw.x, tw.y, tw.z, localTangent.w};
        }

        const GltfDecodedSkinning& skinning = decoded.primitive.skinning[vi];
        SkinnedMesh::Vertex vertex{};
        vertex.position = pw;
        vertex.normal = nw;
        vertex.texCoord = tc;
        vertex.texCoord1 = tc1;
        vertex.color = color;
        vertex.tangent = tangent;
        vertex.joints[0] = skinning.joints[0];
        vertex.joints[1] = skinning.joints[1];
        vertex.joints[2] = skinning.joints[2];
        vertex.joints[3] = skinning.joints[3];
        vertex.weights[0] = skinning.weights[0];
        vertex.weights[1] = skinning.weights[1];
        vertex.weights[2] = skinning.weights[2];
        vertex.weights[3] = skinning.weights[3];
        outMesh.GetVertices().PushBack(vertex);
    }

    if (!decoded.primitive.indices.IsEmpty()) {
        const cgltf_size icount = decoded.primitive.indices.GetSize();
        if (icount % 3 != 0) {
            outcome.ok = false;
            outcome.errorMessage = Utf8String("Decoded skinned glTF primitive index count is not divisible by 3");
            return outcome;
        }
        for (cgltf_size ti = 0; ti < icount; ti += 3) {
            const std::uint32_t i0 = base + decoded.primitive.indices[ti + 0];
            const std::uint32_t i1 = base + decoded.primitive.indices[ti + 1];
            const std::uint32_t i2 = base + decoded.primitive.indices[ti + 2];
            if (flipWinding) {
                outMesh.AddTriangle(i0, i2, i1);
            } else {
                outMesh.AddTriangle(i0, i1, i2);
            }
        }
    } else {
        for (cgltf_size ti = 0; ti + 2 < vertexCount; ti += 3) {
            const std::uint32_t i0 = base + static_cast<std::uint32_t>(ti);
            const std::uint32_t i1 = base + static_cast<std::uint32_t>(ti + 1);
            const std::uint32_t i2 = base + static_cast<std::uint32_t>(ti + 2);
            if (flipWinding) {
                outMesh.AddTriangle(i0, i2, i1);
            } else {
                outMesh.AddTriangle(i0, i1, i2);
            }
        }
    }

    const std::uint32_t indexCount =
            static_cast<std::uint32_t>(outMesh.GetIndices().GetSize()) - indexOffset;
    if (indexCount == 0) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Decoded skinned glTF primitive produced no indices");
        return outcome;
    }

    MeshSubmesh submesh{};
    submesh.indexOffset = indexOffset;
    submesh.indexCount = indexCount;
    submesh.materialIndex = MaterialIndex(data, prim->material);
    outMesh.GetSubmeshes().PushBack(submesh);
    return outcome;
}

GltfMeshBuildOutcome GltfSkinnedMeshBuilder::BuildFromCgltfMesh(
        const cgltf_data* data,
        const cgltf_mesh* mesh,
        const Matrix4& bakeWorld,
        const std::uint32_t texCoordSet,
        bool* outHadNormals,
        bool* outHadTangents,
        SkinnedMesh& outMesh) {
    GltfMeshBuildOutcome outcome{};
    if (data == nullptr || mesh == nullptr) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Null skinned glTF mesh");
        return outcome;
    }
    for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
        const GltfMeshBuildOutcome primitiveOutcome = AppendSkinnedPrimitive(
                data,
                &mesh->primitives[pi],
                bakeWorld,
                texCoordSet,
                outHadNormals,
                outHadTangents,
                outMesh);
        if (!primitiveOutcome.ok) {
            return primitiveOutcome;
        }
    }
    if (outMesh.GetVertices().IsEmpty()) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Skinned glTF mesh has no geometry");
        return outcome;
    }
    return outcome;
}

}  // namespace Spark
