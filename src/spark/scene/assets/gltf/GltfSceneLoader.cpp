#include "spark/scene/assets/gltf/GltfSceneLoader.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/gltf/GltfDataLoader.hpp"
#include "spark/scene/assets/gltf/GltfMeshBuilder.hpp"
#include "spark/scene/assets/gltf/GltfNodeTransforms.hpp"
#include "spark/scene/assets/gltf/GltfSkinNodeLoader.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

void LoadAllMaterials(const cgltf_data* data, const char* path, Array<GltfMaterialDesc>& outMaterials) {
    GltfMaterialLoader::LoadAll(data, path, outMaterials);
}

void LoadVariantNames(const cgltf_data* data, Array<Utf8String>& outVariantNames) {
    GltfMaterialLoader::LoadVariantNames(data, outVariantNames);
}

class MeshTableBuilder {
public:
    explicit MeshTableBuilder(const char* sourcePath) : sourcePath(sourcePath) {}

    [[nodiscard]] const char* GetSourcePath() const noexcept { return sourcePath; }

    void SetLastError(const Utf8String& message) { lastError = message; }

    [[nodiscard]] std::uint32_t GetOrCreateMeshIndex(const cgltf_data* data, const cgltf_mesh* mesh, Array<SharedPtr<Mesh>>& outMeshes) {
        if (mesh == nullptr) {
            return GltfSceneNode::kInvalidMeshIndex;
        }
        if (const std::uint32_t* existing = meshIndices.Find(mesh)) {
            return *existing;
        }
        auto built = MakeShared<Mesh>(Utf8String(sourcePath != nullptr ? sourcePath : "gltf_mesh"));
        const GltfMeshBuildOutcome buildOutcome = GltfMeshBuilder::BuildFromCgltfMesh(data, mesh, *built);
        if (!buildOutcome.ok) {
            lastError = buildOutcome.errorMessage;
            return GltfSceneNode::kInvalidMeshIndex;
        }
        const std::uint32_t index = static_cast<std::uint32_t>(outMeshes.GetSize());
        outMeshes.PushBack(built);
        meshIndices.Add(mesh, index);
        return index;
    }

    [[nodiscard]] const Utf8String& GetLastError() const noexcept { return lastError; }

private:
    struct PointerHasher {
        [[nodiscard]] std::size_t operator()(const cgltf_mesh* ptr) const noexcept {
            return reinterpret_cast<std::size_t>(ptr);
        }
    };

    const char* sourcePath = nullptr;
    Utf8String lastError;
    HashMap<const cgltf_mesh*, std::uint32_t, PointerHasher> meshIndices;
};

void VisitSceneNode(
        const cgltf_data* data,
        cgltf_node* node,
        const std::int32_t parentIndex,
        MeshTableBuilder& meshBuilder,
        GltfSceneDocument& outDocument) {
    if (node == nullptr) {
        return;
    }

    GltfSceneNode sceneNode{};
    if (node->name != nullptr && node->name[0] != '\0') {
        sceneNode.name = Utf8String(node->name);
    }
    sceneNode.localTransform = GltfNodeTransforms::LocalTransform(node);
    sceneNode.parentIndex = parentIndex;
    sceneNode.hasSkin = node->skin != nullptr;
    if (node->mesh != nullptr && node->skin == nullptr) {
        sceneNode.meshIndex = meshBuilder.GetOrCreateMeshIndex(data, node->mesh, outDocument.meshes);
    } else if (node->mesh != nullptr && node->skin != nullptr) {
        const GltfSkinNodeBuildResult built = TryBuildSkinNode(data, node, meshBuilder.GetSourcePath());
        if (!built.ok) {
            meshBuilder.SetLastError(built.errorMessage);
        } else {
            sceneNode.skinnedMeshIndex = static_cast<std::uint32_t>(outDocument.skinnedMeshes.GetSize());
            outDocument.skinnedMeshes.PushBack(built.mesh);
            sceneNode.skeletonIndex = static_cast<std::uint32_t>(outDocument.skeletons.GetSize());
            outDocument.skeletons.PushBack(built.skeleton);
        }
    }

    const std::uint32_t nodeIndex = static_cast<std::uint32_t>(outDocument.nodes.GetSize());
    outDocument.nodes.PushBack(sceneNode);
    if (parentIndex < 0) {
        outDocument.rootNodeIndices.PushBack(nodeIndex);
    }

    for (cgltf_size ci = 0; ci < node->children_count; ++ci) {
        VisitSceneNode(data, node->children[ci], static_cast<std::int32_t>(nodeIndex), meshBuilder, outDocument);
    }
}

void VisitSceneRoots(cgltf_data* data, cgltf_scene* scene, MeshTableBuilder& meshBuilder, GltfSceneDocument& outDocument) {
    if (scene == nullptr) {
        return;
    }
    for (cgltf_size i = 0; i < scene->nodes_count; ++i) {
        VisitSceneNode(data, scene->nodes[i], -1, meshBuilder, outDocument);
    }
}

}  // namespace

AssetLoadOutcome<GltfSceneDocument> GltfSceneLoader::TryLoadFromFile(const char* path) noexcept {
    AssetLoadOutcome<GltfSceneDocument> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    if (path == nullptr || path[0] == '\0') {
        outcome.errorMessage = Utf8String("Empty glTF path");
        return outcome;
    }

    const GltfDataLoadResult loaded = LoadParsedGltfFile(path);
    if (!loaded.ok || loaded.data == nullptr) {
        outcome.errorMessage = loaded.errorMessage.IsEmpty() ? Utf8String("Failed to load glTF file: ") :
                                                               loaded.errorMessage;
        if (outcome.errorMessage.IsEmpty()) {
            outcome.errorMessage.AppendUtf8(path);
        }
        return outcome;
    }
    cgltf_data* data = loaded.data;

    GltfSceneDocument document{};
    document.sourcePath = Utf8String(path);
    MeshTableBuilder meshBuilder(path);

    cgltf_scene* primary = data->scene;
    if (primary == nullptr && data->scenes_count > 0) {
        primary = &data->scenes[0];
    }
    VisitSceneRoots(data, primary, meshBuilder, document);
    if (document.nodes.IsEmpty() && data->scenes_count > 0) {
        for (cgltf_size si = 0; si < data->scenes_count; ++si) {
            if (primary != nullptr && &data->scenes[si] == primary) {
                continue;
            }
            VisitSceneRoots(data, &data->scenes[si], meshBuilder, document);
            if (!document.nodes.IsEmpty()) {
                break;
            }
        }
    }

    LoadAllMaterials(data, path, document.materials);
    LoadVariantNames(data, document.materialVariantNames);
    cgltf_free(data);

    if (!meshBuilder.GetLastError().IsEmpty()) {
        outcome.errorMessage = meshBuilder.GetLastError();
        return outcome;
    }

    if (document.nodes.IsEmpty()) {
        outcome.errorMessage = Utf8String("No nodes found in glTF scene: ");
        outcome.errorMessage.AppendUtf8(path);
        return outcome;
    }

    outcome.ok = true;
    outcome.value = MoveTemp(document);
    return outcome;
}

}  // namespace Spark
