#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/**
 * Resolves and stages glTF files under the project assets tree.
 * Used by <c>GltfPrefabImporter</c> so prefabs store portable relative paths.
 */
class GltfAssetPathResolver final {
public:
    /** Strips known asset roots from <c>path</c> (e.g. <c>models/Foo.glb</c>). */
    [[nodiscard]] static bool TryMakeAssetsRelative(const char* path, Utf8String& outRelative) noexcept;

    /**
     * Ensures <c>sourcePath</c> is addressable from the assets root.
     * Files outside the tree are copied into <c>models/imported/</c>.
     */
    [[nodiscard]] static bool EnsureInAssetLibrary(
            const char* sourcePath,
            Utf8String& outRelative,
            Utf8String* outError = nullptr) noexcept;

    [[nodiscard]] static Utf8String SanitizeBaseName(const char* fileName) noexcept;

private:
    GltfAssetPathResolver() = delete;
};

}  // namespace Spark
