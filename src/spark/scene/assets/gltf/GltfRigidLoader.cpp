#include "spark/scene/assets/gltf/GltfRigidLoader.hpp"

#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"
#include "spark/scene/assets/gltf/GltfNodeTransforms.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

void VisitNode(cgltf_data* data, cgltf_node* node, const Matrix4& parentWorld, Mesh& outMesh) {
    if (node == nullptr) {
        return;
    }
    const Matrix4 local = GltfNodeTransforms::LocalTransform(node).ToMatrix4();
    const Matrix4 world = parentWorld * local;

    if (node->mesh != nullptr && node->skin == nullptr) {
        const cgltf_mesh* mesh = node->mesh;
        for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
            GltfMeshBuilder::AppendPrimitive(data, &mesh->primitives[pi], world, outMesh);
        }
    }
    for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
        VisitNode(data, node->children[ci], world, outMesh);
    }
}

void LoadAllMaterials(const cgltf_data* data, const char* path, Array<GltfMaterial>& outMaterials) {
    GltfMaterialLoader::LoadAll(data, path, outMaterials);
}

}  // namespace

bool GltfRigidLoader::LoadFromFile(const char* path, GltfRigidLoadResult& out) noexcept {
    out = GltfRigidLoadResult{};
    if (path == nullptr || path[0] == '\0') {
        out.errorMessage = Utf8String("Empty glTF path");
        return false;
    }
    out.errorMessage = Utf8String(path);

    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || data == nullptr) {
        out.errorMessage.AppendUtf8(": failed to parse glTF file");
        return false;
    }
    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        out.errorMessage.AppendUtf8(": failed to load glTF buffers");
        return false;
    }

    auto mesh = MakeShared<Mesh>(Utf8String(path));
    mesh->Clear();

    cgltf_scene* primary = data->scene;
    if (primary == nullptr && data->scenes_count > 0) {
        primary = &data->scenes[0];
    }
    auto visitSceneRoots = [&](cgltf_scene* sc) {
        if (sc == nullptr) {
            return;
        }
        for (cgltf_size i = 0; i < sc->nodes_count; ++i) {
            VisitNode(data, sc->nodes[i], Matrix4::Identity, *mesh);
        }
    };
    visitSceneRoots(primary);
    if (mesh->GetVertices().IsEmpty() && data->scenes_count > 0) {
        for (cgltf_size si = 0; si < data->scenes_count; ++si) {
            if (primary != nullptr && &data->scenes[si] == primary) {
                continue;
            }
            visitSceneRoots(&data->scenes[si]);
            if (!mesh->GetVertices().IsEmpty()) {
                break;
            }
        }
    }

    if (mesh->GetVertices().IsEmpty()) {
        cgltf_free(data);
        out.errorMessage.AppendUtf8(": no mesh geometry found in glTF");
        return false;
    }

    LoadAllMaterials(data, path, out.materials);
    cgltf_free(data);

    out.mesh = mesh;
    out.success = true;
    out.errorMessage.Clear();
    return true;
}

}  // namespace Spark
