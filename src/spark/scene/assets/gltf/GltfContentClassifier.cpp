#include "spark/scene/assets/gltf/GltfContentClassifier.hpp"

#include "spark/scene/assets/gltf/GltfDataLoader.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

void ScanForSkinnedMeshNode(const cgltf_node* node, const cgltf_node** outSkinNode) {
    if (node == nullptr || outSkinNode == nullptr || *outSkinNode != nullptr) {
        return;
    }
    if (node->mesh != nullptr && node->skin != nullptr) {
        *outSkinNode = node;
        return;
    }
    for (cgltf_size c = 0; c < node->children_count; ++c) {
        ScanForSkinnedMeshNode(node->children[c], outSkinNode);
    }
}

[[nodiscard]] GltfContentKind ClassifyParsedData(const cgltf_data* data) noexcept {
    if (data == nullptr || data->skins_count == 0) {
        return GltfContentKind::Rigid;
    }
    const cgltf_node* skinnedNode = nullptr;
    if (data->scene != nullptr) {
        for (cgltf_size i = 0; i < data->scene->nodes_count; ++i) {
            ScanForSkinnedMeshNode(data->scene->nodes[i], &skinnedNode);
            if (skinnedNode != nullptr) {
                break;
            }
        }
    } else {
        for (cgltf_size i = 0; i < data->nodes_count; ++i) {
            ScanForSkinnedMeshNode(&data->nodes[i], &skinnedNode);
            if (skinnedNode != nullptr) {
                break;
            }
        }
    }
    return skinnedNode != nullptr ? GltfContentKind::Skinned : GltfContentKind::Rigid;
}

void ScanCompression(const cgltf_data* data, GltfCompressionFlags& outFlags) noexcept {
    if (data == nullptr) {
        return;
    }
    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi) {
        const cgltf_mesh& mesh = data->meshes[mi];
        for (cgltf_size pi = 0; pi < mesh.primitives_count; ++pi) {
            if (mesh.primitives[pi].has_draco_mesh_compression) {
                outFlags.draco = true;
            }
        }
    }
    for (cgltf_size bi = 0; bi < data->buffer_views_count; ++bi) {
        if (data->buffer_views[bi].has_meshopt_compression) {
            outFlags.meshopt = true;
        }
    }
}

}  // namespace

GltfContentClassifier::ProbeResult GltfContentClassifier::ProbeFile(const char* path) {
    ProbeResult result{};
    if (path == nullptr || path[0] == '\0') {
        result.errorMessage = Utf8String("Empty glTF path");
        return result;
    }

    const GltfDataLoadResult loaded = LoadParsedGltfFile(path);
    if (!loaded.ok || loaded.data == nullptr) {
        result.errorMessage = loaded.errorMessage.IsEmpty() ? Utf8String("Failed to parse glTF file") : loaded.errorMessage;
        return result;
    }

    result.parseOk = true;
    result.kind = ClassifyParsedData(loaded.data);
    ScanCompression(loaded.data, result.compression);
    cgltf_free(loaded.data);
    return result;
}

}  // namespace Spark
