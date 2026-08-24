#include "spark/scene/assets/gltf/GltfContentClassifier.hpp"

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

}  // namespace

GltfContentClassifier::ProbeResult GltfContentClassifier::ProbeFile(const char* path) {
    ProbeResult result{};
    if (path == nullptr || path[0] == '\0') {
        result.errorMessage = Utf8String("Empty glTF path");
        return result;
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || data == nullptr) {
        result.errorMessage = Utf8String("Failed to parse glTF file");
        return result;
    }

    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        result.errorMessage = Utf8String("Failed to load glTF buffers");
        return result;
    }

    result.parseOk = true;
    result.kind = ClassifyParsedData(data);
    cgltf_free(data);
    return result;
}

}  // namespace Spark
