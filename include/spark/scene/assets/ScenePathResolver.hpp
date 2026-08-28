#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/**
 * Resolves content-pipeline paths under source and build asset roots.
 * Mirrors the fallback strategy used by <c>ResolveTilemapAssetPath</c>.
 */
class ScenePathResolver final {
public:
    [[nodiscard]] static bool FileExists(const char* path) noexcept;

    /** Absolute path under <c>SPARK_BUILD_ASSETS_DIR</c>. */
    [[nodiscard]] static Utf8String BuildRuntimePath(const char* subdir, const char* fileName) noexcept;

    /** Absolute path under <c>SPARK_ASSETS_DIR</c>. */
    [[nodiscard]] static Utf8String SourceAssetPath(const char* subdir, const char* fileName) noexcept;

    [[nodiscard]] static const char* AssetsRoot() noexcept;
    [[nodiscard]] static const char* BuildAssetsRoot() noexcept;

    [[nodiscard]] static Utf8String ResolveReadablePath(const char* relativeOrAbsolutePath) noexcept;

private:
    ScenePathResolver() = delete;
};

}  // namespace Spark
