#include "spark/scene/assets/gltf/GltfRigidLoader.hpp"

#include "spark/scene/assets/gltf/GltfDataLoader.hpp"
#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"
#include "spark/scene/assets/gltf/GltfNodeTransforms.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

bool VisitNode(cgltf_data* data, cgltf_node* node, const Matrix4& parentWorld, Mesh& outMesh, Utf8String& outError) {
    if (node == nullptr) {
        return true;
    }
    const Matrix4 local = GltfNodeTransforms::LocalTransform(node).ToMatrix4();
    const Matrix4 world = parentWorld * local;

    if (node->mesh != nullptr && node->skin == nullptr) {
        const cgltf_mesh* mesh = node->mesh;
        for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi) {
            const GltfMeshBuildOutcome primitiveOutcome =
                    GltfMeshBuilder::AppendPrimitive(data, &mesh->primitives[pi], world, outMesh);
            if (!primitiveOutcome.ok) {
                outError = primitiveOutcome.errorMessage;
                return false;
            }
        }
    }
    for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
        if (!VisitNode(data, node->children[ci], world, outMesh, outError)) {
            return false;
        }
    }
    return true;
}

void LoadAllMaterials(const cgltf_data* data, const char* path, Array<GltfMaterial>& outMaterials) {
    GltfMaterialLoader::LoadAll(data, path, outMaterials);
}

void LoadVariantNames(const cgltf_data* data, Array<Utf8String>& outVariantNames) {
    GltfMaterialLoader::LoadVariantNames(data, outVariantNames);
}

}  // namespace

bool GltfRigidLoader::LoadFromFile(const char* path, GltfRigidLoadResult& out) noexcept {
    out = GltfRigidLoadResult{};
    if (path == nullptr || path[0] == '\0') {
        out.errorMessage = Utf8String("Empty glTF path");
        return false;
    }
    out.errorMessage = Utf8String(path);

    const GltfDataLoadResult loaded = LoadParsedGltfFile(path);
    if (!loaded.ok || loaded.data == nullptr) {
        out.errorMessage.AppendUtf8(": ");
        out.errorMessage.AppendUtf8(
                loaded.errorMessage.IsEmpty() ? "failed to load glTF file" : loaded.errorMessage.CStr());
        return false;
    }
    cgltf_data* data = loaded.data;

    auto mesh = MakeShared<Mesh>(Utf8String(path));
    mesh->Clear();

    cgltf_scene* primary = data->scene;
    if (primary == nullptr && data->scenes_count > 0) {
        primary = &data->scenes[0];
    }
    Utf8String meshError;
    auto visitSceneRoots = [&](cgltf_scene* sc) -> bool {
        if (sc == nullptr) {
            return true;
        }
        for (cgltf_size i = 0; i < sc->nodes_count; ++i) {
            if (!VisitNode(data, sc->nodes[i], Matrix4::Identity, *mesh, meshError)) {
                return false;
            }
        }
        return true;
    };
    if (!visitSceneRoots(primary)) {
        cgltf_free(data);
        out.errorMessage.AppendUtf8(": ");
        out.errorMessage.AppendUtf8(meshError.CStr());
        return false;
    }
    if (mesh->GetVertices().IsEmpty() && data->scenes_count > 0) {
        for (cgltf_size si = 0; si < data->scenes_count; ++si) {
            if (primary != nullptr && &data->scenes[si] == primary) {
                continue;
            }
            if (!visitSceneRoots(&data->scenes[si])) {
                cgltf_free(data);
                out.errorMessage.AppendUtf8(": ");
                out.errorMessage.AppendUtf8(meshError.CStr());
                return false;
            }
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

    Mesh::RecomputeTangentSpace(*mesh);

    LoadAllMaterials(data, path, out.materials);
    LoadVariantNames(data, out.materialVariantNames);
    cgltf_free(data);

    out.mesh = mesh;
    out.success = true;
    out.errorMessage.Clear();
    return true;
}

}  // namespace Spark
