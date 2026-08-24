#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"

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

void GltfMeshBuilder::AppendPrimitive(
        const cgltf_data* data,
        const cgltf_primitive* prim,
        const Matrix4& transform,
        Mesh& outMesh) {
    if (data == nullptr || prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
        return;
    }
    const cgltf_accessor* pos = nullptr;
    const cgltf_accessor* nrm = nullptr;
    const cgltf_accessor* uv = nullptr;
    for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai) {
        const cgltf_attribute& a = prim->attributes[ai];
        if (a.type == cgltf_attribute_type_position) {
            pos = a.data;
        } else if (a.type == cgltf_attribute_type_normal) {
            nrm = a.data;
        } else if (a.type == cgltf_attribute_type_texcoord && a.index == 0) {
            uv = a.data;
        }
    }
    if (pos == nullptr || pos->type != cgltf_type_vec3) {
        return;
    }

    const std::uint32_t indexOffset = static_cast<std::uint32_t>(outMesh.GetIndices().GetSize());
    const cgltf_size vcount = pos->count;
    const std::uint32_t base = static_cast<std::uint32_t>(outMesh.GetVertices().GetSize());
    const bool flipWinding = ShouldFlipGltfTriangleWinding(transform);

    for (cgltf_size vi = 0; vi < vcount; ++vi) {
        float p[3]{};
        cgltf_accessor_read_float(pos, vi, p, 3);
        const Vector3 pw = transform.TransformPoint({p[0], p[1], p[2]});
        Vector3 nw{0.0F, 1.0F, 0.0F};
        if (nrm != nullptr && nrm->type == cgltf_type_vec3) {
            float n[3]{};
            cgltf_accessor_read_float(nrm, vi, n, 3);
            nw = transform.TransformVector({n[0], n[1], n[2]}).Normalized();
        }
        Vector2 tc{0.0F, 0.0F};
        if (uv != nullptr && uv->type == cgltf_type_vec2) {
            float t[2]{};
            cgltf_accessor_read_float(uv, vi, t, 2);
            tc = {t[0], t[1]};
        }
        outMesh.AddVertex({pw, nw, tc});
    }

    if (prim->indices != nullptr) {
        const cgltf_accessor* idx = prim->indices;
        const cgltf_size icount = idx->count;
        if (icount % 3 != 0) {
            return;
        }
        for (cgltf_size ti = 0; ti < icount; ti += 3) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 0));
            const std::uint32_t i1 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 1));
            const std::uint32_t i2 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 2));
            if (flipWinding) {
                outMesh.AddTriangle(base + i0, base + i2, base + i1);
            } else {
                outMesh.AddTriangle(base + i0, base + i1, base + i2);
            }
        }
    } else {
        for (cgltf_size ti = 0; ti + 2 < vcount; ti += 3) {
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
        return;
    }
    MeshSubmesh submesh{};
    submesh.indexOffset = indexOffset;
    submesh.indexCount = indexCount;
    submesh.materialIndex = MaterialIndex(data, prim->material);
    outMesh.GetSubmeshes().PushBack(submesh);
}

bool GltfMeshBuilder::BuildFromCgltfMesh(const cgltf_data* data, const cgltf_mesh* mesh, Mesh& outMesh) {
    if (data == nullptr || mesh == nullptr) {
        return false;
    }
    outMesh.Clear();
    for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
        AppendPrimitive(data, &mesh->primitives[pi], Matrix4::Identity, outMesh);
    }
    return !outMesh.GetVertices().IsEmpty();
}

}  // namespace Spark
