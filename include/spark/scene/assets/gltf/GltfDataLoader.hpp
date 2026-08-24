#pragma once

#include "spark/core/Utf8String.hpp"

struct cgltf_data;
struct cgltf_options;

namespace Spark {

struct GltfDataLoadResult {
    bool ok = false;
    Utf8String errorMessage;
    cgltf_data* data = nullptr;
};

/** Parses a glTF file, loads external buffers, and decompresses meshopt buffer views. */
[[nodiscard]] GltfDataLoadResult LoadParsedGltfFile(const char* path) noexcept;
[[nodiscard]] GltfDataLoadResult LoadParsedGltfFile(const char* path, const cgltf_options& options) noexcept;

}  // namespace Spark
