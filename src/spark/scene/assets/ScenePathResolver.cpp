#include "spark/scene/assets/ScenePathResolver.hpp"

#include "spark/config.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

bool ScenePathResolver::FileExists(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    std::fclose(file);
    return true;
}

const char* ScenePathResolver::AssetsRoot() noexcept {
    return SPARK_ASSETS_DIR;
}

const char* ScenePathResolver::BuildAssetsRoot() noexcept {
    return SPARK_BUILD_ASSETS_DIR;
}

Utf8String ScenePathResolver::BuildRuntimePath(const char* subdir, const char* fileName) noexcept {
    if (fileName == nullptr || fileName[0] == '\0') {
        return {};
    }
    char buffer[1024]{};
    if (subdir != nullptr && subdir[0] != '\0') {
        const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s/%s", SPARK_BUILD_ASSETS_DIR, subdir, fileName);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
            return {};
        }
    } else {
        const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s", SPARK_BUILD_ASSETS_DIR, fileName);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
            return {};
        }
    }
    return Utf8String(buffer);
}

Utf8String ScenePathResolver::SourceAssetPath(const char* subdir, const char* fileName) noexcept {
    if (fileName == nullptr || fileName[0] == '\0') {
        return {};
    }
    char buffer[1024]{};
    if (subdir != nullptr && subdir[0] != '\0') {
        const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s/%s", SPARK_ASSETS_DIR, subdir, fileName);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
            return {};
        }
    } else {
        const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s", SPARK_ASSETS_DIR, fileName);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
            return {};
        }
    }
    return Utf8String(buffer);
}

namespace {

Utf8String TryReadable(const char* path) {
    if (ScenePathResolver::FileExists(path)) {
        return Utf8String(path);
    }
    return {};
}

Utf8String JoinRoot(const char* root, const char* relative) {
    if (root == nullptr || relative == nullptr || relative[0] == '\0') {
        return {};
    }
    while (relative[0] == '/') {
        ++relative;
    }
    char buffer[1024]{};
    const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s", root, relative);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
        return {};
    }
    return TryReadable(buffer);
}

}  // namespace

Utf8String ScenePathResolver::ResolveReadablePath(const char* relativeOrAbsolutePath) noexcept {
    if (relativeOrAbsolutePath == nullptr || relativeOrAbsolutePath[0] == '\0') {
        return {};
    }
    if (Utf8String direct = TryReadable(relativeOrAbsolutePath); !direct.IsEmpty()) {
        return direct;
    }
    if (Utf8String fromSource = JoinRoot(SPARK_ASSETS_DIR, relativeOrAbsolutePath); !fromSource.IsEmpty()) {
        return fromSource;
    }
    if (Utf8String fromBuild = JoinRoot(SPARK_BUILD_ASSETS_DIR, relativeOrAbsolutePath); !fromBuild.IsEmpty()) {
        return fromBuild;
    }
    if (std::strncmp(relativeOrAbsolutePath, "assets/", 7) == 0) {
        const char* tail = relativeOrAbsolutePath + 7;
        if (Utf8String nested = JoinRoot(SPARK_ASSETS_DIR, tail); !nested.IsEmpty()) {
            return nested;
        }
    }
    return JoinRoot("assets", relativeOrAbsolutePath);
}

}  // namespace Spark
