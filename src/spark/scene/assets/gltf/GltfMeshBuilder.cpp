#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"

#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

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

GltfMeshBuildOutcome GltfMeshBuilder::AppendPrimitive(
        const cgltf_data* data,
        const cgltf_primitive* prim,
        const Matrix4& transform,
        Mesh& outMesh) {
    GltfMeshBuildOutcome outcome{};
    if (data == nullptr || prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Unsupported glTF primitive");
        return outcome;
    }

    const GltfPrimitiveDecodeResult decoded = GltfPrimitiveDecoderRegistry::Decode(data, prim);
    if (!decoded.ok) {
        outcome.ok = false;
        outcome.errorMessage = decoded.errorMessage;
        return outcome;
    }
    if (decoded.primitive.positions.IsEmpty()) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Decoded glTF primitive has no vertices");
        return outcome;
    }

    const std::uint32_t indexOffset = static_cast<std::uint32_t>(outMesh.GetIndices().GetSize());
    const std::uint32_t base = static_cast<std::uint32_t>(outMesh.GetVertices().GetSize());
    const bool flipWinding = ShouldFlipGltfTriangleWinding(transform);
    const cgltf_size vertexCount = decoded.primitive.positions.GetSize();
    const bool hasNormals = decoded.primitive.normals.GetSize() == vertexCount;
    const bool hasTexcoords = decoded.primitive.texcoords.GetSize() == vertexCount;
    const bool hasTexcoords1 = decoded.primitive.texcoords1.GetSize() == vertexCount;
    const bool hasColors = decoded.primitive.colors.GetSize() == vertexCount;

    for (cgltf_size vi = 0; vi < vertexCount; ++vi) {
        const Vector3 pw = transform.TransformPoint(decoded.primitive.positions[vi]);
        Vector3 nw{0.0F, 1.0F, 0.0F};
        if (hasNormals) {
            nw = transform.TransformVector(decoded.primitive.normals[vi]).Normalized();
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
        Mesh::Vertex vertex{};
        vertex.position = pw;
        vertex.normal = nw;
        vertex.texCoord = tc;
        vertex.texCoord1 = tc1;
        vertex.color = color;
        outMesh.AddVertex(vertex);
    }

    if (!decoded.primitive.indices.IsEmpty()) {
        const cgltf_size icount = decoded.primitive.indices.GetSize();
        if (icount % 3 != 0) {
            outcome.ok = false;
            outcome.errorMessage = Utf8String("Decoded glTF primitive index count is not divisible by 3");
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
        outcome.errorMessage = Utf8String("Decoded glTF primitive produced no indices");
        return outcome;
    }

    MeshSubmesh submesh{};
    submesh.indexOffset = indexOffset;
    submesh.indexCount = indexCount;
    submesh.materialIndex = MaterialIndex(data, prim->material);
    for (cgltf_size mi = 0; mi < prim->mappings_count; ++mi) {
        MeshSubmeshVariantMapping mapping{};
        mapping.variantIndex = static_cast<std::uint32_t>(prim->mappings[mi].variant);
        mapping.materialIndex = MaterialIndex(data, prim->mappings[mi].material);
        submesh.variantMappings.PushBack(mapping);
    }
    outMesh.GetSubmeshes().PushBack(submesh);
    return outcome;
}

GltfMeshBuildOutcome GltfMeshBuilder::BuildFromCgltfMesh(
        const cgltf_data* data,
        const cgltf_mesh* mesh,
        Mesh& outMesh) {
    GltfMeshBuildOutcome outcome{};
    if (data == nullptr || mesh == nullptr) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("Null glTF mesh");
        return outcome;
    }
    outMesh.Clear();
    for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
        const GltfMeshBuildOutcome primitiveOutcome =
                AppendPrimitive(data, &mesh->primitives[pi], Matrix4::Identity, outMesh);
        if (!primitiveOutcome.ok) {
            return primitiveOutcome;
        }
    }
    if (outMesh.GetVertices().IsEmpty()) {
        outcome.ok = false;
        outcome.errorMessage = Utf8String("glTF mesh has no geometry");
        return outcome;
    }
    Mesh::RecomputeTangentSpace(outMesh);
    return outcome;
}

}  // namespace Spark
