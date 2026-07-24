#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Returns true when <c>path</c> can be opened for read. */
[[nodiscard]] bool TilemapFileExists(const char* path) noexcept;

/**
 * Finds a readable file: absolute/relative as given, then under <c>SPARK_ASSETS_DIR</c>,
 * <c>SPARK_BUILD_ASSETS_DIR</c>, and common <c>assets/</c> fallbacks.
 */
[[nodiscard]] Utf8String ResolveTilemapAssetPath(const char* relativeOrAbsolutePath) noexcept;

}  // namespace Spark
