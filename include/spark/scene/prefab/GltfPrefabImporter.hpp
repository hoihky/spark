#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"

namespace Spark {

class GameWorld;

/** Options when baking a glTF file into a <c>.sparkscene</c> prefab. */
struct GltfPrefabImportOptions {
    /** Target maximum axis extent in world units (uniform scale on root transform). */
    float targetMaxExtent = 2.2F;
    /** Relative output directory under both source and build asset roots. */
    const char* outputSubdir = "prefabs";
    /** Written into prefab header <c>assets_root</c>. */
    const char* assetsRoot = "assets";
    /** Optional override; defaults to sanitized glTF base name. */
    Utf8String prefabBaseName{};
};

/** Result of <c>GltfPrefabImporter::ImportFromGltf</c>. */
struct GltfPrefabImportResult {
    bool ok = false;
    Utf8String errorMessage{};
    Utf8String gltfAssetRel{};
    Utf8String prefabFileName{};
    Utf8String prefabCaptureHint{};
    PrefabCatalogEntry catalogEntry{};
};

/**
 * Builds portable <c>.sparkscene</c> prefabs from arbitrary glTF/glB files.
 * Prefabs reference a <c>gltf_scene</c> component that expands on instantiation.
 */
class GltfPrefabImporter final {
public:
    /**
     * Validates the glTF, stages it under assets when needed, and writes prefab files
     * to source and build asset roots.
     */
    [[nodiscard]] GltfPrefabImportResult ImportFromGltf(
            GameWorld& world,
            const char* gltfPath,
            const GltfPrefabImportOptions& options = {}) const;
};

}  // namespace Spark
