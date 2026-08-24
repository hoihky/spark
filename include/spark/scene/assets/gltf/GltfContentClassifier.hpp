#pragma once

#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

/** High-level glTF scene content detected from a file header parse. */
enum class GltfContentKind : std::uint8_t {
    Unknown = 0,
    Rigid,
    Skinned,
};

/**
 * Inspects a glTF file without building meshes (Strategy entry point for loader routing).
 * Used by <c>GltfAssetBinder::BindFromPath</c> and rigid-load guards.
 */
class GltfContentClassifier {
public:
    struct ProbeResult {
        GltfContentKind kind = GltfContentKind::Unknown;
        bool parseOk = false;
        Utf8String errorMessage;
    };

    [[nodiscard]] static ProbeResult ProbeFile(const char* path);
};

}  // namespace Spark
