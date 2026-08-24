#include "spark/scene/assets/gltf/GltfDataLoader.hpp"

#include "spark/scene/assets/gltf/GltfMeshoptBuffers.hpp"

#include "cgltf.h"

namespace Spark {

GltfDataLoadResult LoadParsedGltfFile(const char* path) noexcept {
    cgltf_options options{};
    return LoadParsedGltfFile(path, options);
}

GltfDataLoadResult LoadParsedGltfFile(const char* path, const cgltf_options& options) noexcept {
    GltfDataLoadResult result{};
    if (path == nullptr || path[0] == '\0') {
        result.errorMessage = Utf8String("Empty glTF path");
        return result;
    }

    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || data == nullptr) {
        result.errorMessage = Utf8String("Failed to parse glTF file: ");
        result.errorMessage.AppendUtf8(path);
        return result;
    }
    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        result.errorMessage = Utf8String("Failed to load glTF buffers: ");
        result.errorMessage.AppendUtf8(path);
        return result;
    }
    if (!GltfMeshoptBuffers::TryDecodeAll(data, result.errorMessage)) {
        cgltf_free(data);
        return result;
    }

    result.ok = true;
    result.data = data;
    return result;
}

}  // namespace Spark
