#include "spark/scene/Mesh.hpp"
#include "spark/scene/Texture2D.hpp"

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/memory/SharedPtr.hpp"

#include "cgltf.h"

#include <cstring>

namespace Spark {

namespace {

Utf8String ParentDirectory(const char* filePath) {
    if (filePath == nullptr) {
        return {};
    }
    const char* last = nullptr;
    for (const char* p = filePath; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            last = p;
        }
    }
    if (last == nullptr) {
        return {};
    }
    Utf8String out;
    for (const char* q = filePath; q < last; ++q) {
        const char unit[2] = {*q, '\0'};
        out.AppendUtf8(unit);
    }
    return out;
}

void VisitNode(cgltf_node* node, const Matrix4& parentWorld, Mesh& outMesh);

/**
 * glTF front faces are CCW; built-in meshes use the opposite winding for Vulkan PerspectiveVulkan +
 * back-face culling. Mirror indices on import unless bakeWorld already mirrors (det < 0).
 */
bool ShouldFlipGltfTriangleWinding(const Matrix4& bakeWorld) noexcept {
    return bakeWorld.DeterminantUpper3x3() >= 0.0F;
}

void AppendPrimitive(const cgltf_primitive* prim, const Matrix4& world, Mesh& outMesh) {
    if (prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
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

    const cgltf_size vcount = pos->count;
    const std::uint32_t base = static_cast<std::uint32_t>(outMesh.GetVertices().GetSize());
    const bool flipWinding = ShouldFlipGltfTriangleWinding(world);

    for (cgltf_size vi = 0; vi < vcount; ++vi) {
        float p[3]{};
        cgltf_accessor_read_float(pos, vi, p, 3);
        const Vector3 pw = world.TransformPoint({p[0], p[1], p[2]});
        Vector3 nw{0.0F, 1.0F, 0.0F};
        if (nrm != nullptr && nrm->type == cgltf_type_vec3) {
            float n[3]{};
            cgltf_accessor_read_float(nrm, vi, n, 3);
            nw = world.TransformVector({n[0], n[1], n[2]}).Normalized();
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
}

void VisitNode(cgltf_node* node, const Matrix4& parentWorld, Mesh& outMesh) {
    if (node == nullptr) {
        return;
    }
    cgltf_float lm[16]{};
    cgltf_node_transform_local(node, lm);
    Matrix4 local{};
    for (int i = 0; i < 16; ++i) {
        local.m[i] = static_cast<float>(lm[i]);
    }
    const Matrix4 world = parentWorld * local;

    if (node->mesh != nullptr) {
        const cgltf_mesh* mesh = node->mesh;
        for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
            AppendPrimitive(&mesh->primitives[pi], world, outMesh);
        }
    }
    for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
        VisitNode(node->children[ci], world, outMesh);
    }
}

bool TryDecodeGltfImage(const cgltf_image* img, const Utf8String& dir, Texture2D& outDecoded) {
    if (img == nullptr) {
        return false;
    }
    if (img->buffer_view != nullptr) {
        const cgltf_buffer_view* bv = img->buffer_view;
        if (bv->buffer == nullptr || bv->buffer->data == nullptr) {
            return false;
        }
        const auto* bytes =
                static_cast<const std::uint8_t*>(bv->buffer->data) + static_cast<std::size_t>(bv->offset);
        const std::size_t sz = static_cast<std::size_t>(bv->size);
        return Texture2D::TryLoadFromMemory(bytes, sz, outDecoded, "glTF");
    }
    if (img->uri != nullptr) {
        if (std::strncmp(img->uri, "data:", 5) == 0) {
            return false;
        }
        Utf8String full;
        if (dir.IsEmpty()) {
            full = Utf8String(img->uri);
        } else {
            full.AppendUtf8(dir.CStr());
            full.AppendUtf8("/");
            full.AppendUtf8(img->uri);
        }
        return Texture2D::TryLoadFromFile(full.CStr(), outDecoded, false);
    }
    return false;
}

bool TryDecodeTextureView(const cgltf_texture_view& tv, const Utf8String& dir, Texture2D& outDecoded) {
    if (tv.texture == nullptr || tv.texture->image == nullptr) {
        return false;
    }
    return TryDecodeGltfImage(tv.texture->image, dir, outDecoded);
}

/**
 * Picks a single RGBA texture for demo shading: MR baseColor, then KHR_materials_sheen sheenColorTexture
 * (used heavily by SheenChair), then specular-glossiness diffuseTexture.
 */
bool TryLoadFirstBaseColorTexture(
        cgltf_data* data, const char* gltfPath, SharedPtr<Texture2D>* outBaseColor) {
    if (outBaseColor == nullptr || data == nullptr) {
        return false;
    }
    outBaseColor->Reset();
    const Utf8String dir = ParentDirectory(gltfPath);

    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        const cgltf_material& mat = data->materials[mi];
        if (!mat.has_pbr_metallic_roughness) {
            continue;
        }
        Texture2D decoded;
        if (TryDecodeTextureView(mat.pbr_metallic_roughness.base_color_texture, dir, decoded)) {
            *outBaseColor = SharedPtr<Texture2D>(new Texture2D(MoveTemp(decoded)));
            return true;
        }
    }

    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        const cgltf_material& mat = data->materials[mi];
        if (!mat.has_sheen) {
            continue;
        }
        Texture2D decoded;
        if (TryDecodeTextureView(mat.sheen.sheen_color_texture, dir, decoded)) {
            *outBaseColor = SharedPtr<Texture2D>(new Texture2D(MoveTemp(decoded)));
            return true;
        }
    }

    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        const cgltf_material& mat = data->materials[mi];
        if (!mat.has_pbr_specular_glossiness) {
            continue;
        }
        Texture2D decoded;
        if (TryDecodeTextureView(mat.pbr_specular_glossiness.diffuse_texture, dir, decoded)) {
            *outBaseColor = SharedPtr<Texture2D>(new Texture2D(MoveTemp(decoded)));
            return true;
        }
    }

    return false;
}

}  // namespace

bool Mesh::TryLoadFromGltf(const char* path, Mesh& outMesh, SharedPtr<Texture2D>* outBaseColor) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || data == nullptr) {
        return false;
    }
    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }

    outMesh.Clear();
    outMesh.GetName() = Utf8String(path);

    cgltf_scene* primary = data->scene;
    if (primary == nullptr && data->scenes_count > 0) {
        primary = &data->scenes[0];
    }
    auto visitSceneRoots = [&](cgltf_scene* sc) {
        if (sc == nullptr) {
            return;
        }
        for (cgltf_size i = 0; i < sc->nodes_count; ++i) {
            VisitNode(sc->nodes[i], Matrix4::Identity, outMesh);
        }
    };
    visitSceneRoots(primary);
    if (outMesh.GetVertices().IsEmpty() && data->scenes_count > 0) {
        for (cgltf_size si = 0; si < data->scenes_count; ++si) {
            if (primary != nullptr && &data->scenes[si] == primary) {
                continue;
            }
            visitSceneRoots(&data->scenes[si]);
            if (!outMesh.GetVertices().IsEmpty()) {
                break;
            }
        }
    }

    if (outBaseColor != nullptr) {
        TryLoadFirstBaseColorTexture(data, path, outBaseColor);
    }

    cgltf_free(data);
    return !outMesh.GetVertices().IsEmpty();
}

}  // namespace Spark
