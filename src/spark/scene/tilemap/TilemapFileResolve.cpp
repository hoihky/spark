#include "spark/scene/tilemap/TilemapFileResolve.hpp"

#include "spark/config.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

bool TilemapFileExists(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    std::fclose(f);
    return true;
}

namespace {

Utf8String TryPath(const char* path) {
    if (TilemapFileExists(path)) {
        return Utf8String(path);
    }
    return {};
}

Utf8String JoinRootAndRelative(const char* root, const char* relative) {
    if (root == nullptr || relative == nullptr || relative[0] == '\0') {
        return {};
    }
    while (relative[0] == '/') {
        ++relative;
    }
    char buf[1024]{};
    const int n = std::snprintf(buf, sizeof(buf), "%s/%s", root, relative);
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(buf)) {
        return {};
    }
    return TryPath(buf);
}

}  // namespace

Utf8String ResolveTilemapAssetPath(const char* relativeOrAbsolutePath) noexcept {
    if (relativeOrAbsolutePath == nullptr || relativeOrAbsolutePath[0] == '\0') {
        return {};
    }
    if (Utf8String direct = TryPath(relativeOrAbsolutePath); !direct.IsEmpty()) {
        return direct;
    }
    if (Utf8String fromAssets = JoinRootAndRelative(SPARK_ASSETS_DIR, relativeOrAbsolutePath); !fromAssets.IsEmpty()) {
        return fromAssets;
    }
    if (Utf8String fromBuild = JoinRootAndRelative(SPARK_BUILD_ASSETS_DIR, relativeOrAbsolutePath);
        !fromBuild.IsEmpty()) {
        return fromBuild;
    }
    if (std::strncmp(relativeOrAbsolutePath, "assets/", 7) == 0) {
        const char* tail = relativeOrAbsolutePath + 7;
        if (Utf8String nested = JoinRootAndRelative(SPARK_ASSETS_DIR, tail); !nested.IsEmpty()) {
            return nested;
        }
    }
    return JoinRootAndRelative("assets", relativeOrAbsolutePath);
}

}  // namespace Spark
