#include "spark/scene/assets/GltfAssetPathResolver.hpp"

#include "spark/config.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"

#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Spark {

namespace {

bool IsPathSeparator(const char ch) noexcept {
    return ch == '/' || ch == '\\';
}

bool PathsEqualInsensitive(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(*a)));
        const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(*b)));
        const char na = IsPathSeparator(ca) ? '/' : ca;
        const char nb = IsPathSeparator(cb) ? '/' : cb;
        if (na != nb) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

bool StartsWithRoot(const char* path, const char* root) noexcept {
    if (path == nullptr || root == nullptr || root[0] == '\0') {
        return false;
    }
    const std::size_t rootLen = std::strlen(root);
    if (std::strlen(path) < rootLen) {
        return false;
    }
    if (!PathsEqualInsensitive(path, root)) {
        return false;
    }
    const char next = path[rootLen];
    return next == '\0' || IsPathSeparator(next);
}

Utf8String StripRootPrefix(const char* path, const char* root) noexcept {
    if (!StartsWithRoot(path, root)) {
        return {};
    }
    const std::size_t rootLen = std::strlen(root);
    const char* tail = path + rootLen;
    while (*tail != '\0' && IsPathSeparator(*tail)) {
        ++tail;
    }
    return Utf8String(tail);
}

bool CopyFileBinary(const char* sourcePath, const char* destinationPath, Utf8String& outError) noexcept {
    std::FILE* source = std::fopen(sourcePath, "rb");
    if (source == nullptr) {
        outError = Utf8String("Could not read source glTF file.");
        return false;
    }
    std::FILE* destination = std::fopen(destinationPath, "wb");
    if (destination == nullptr) {
        std::fclose(source);
        outError = Utf8String("Could not write imported glTF into assets.");
        return false;
    }
    std::array<char, 65536> buffer{};
    while (true) {
        const std::size_t readCount = std::fread(buffer.data(), 1, buffer.size(), source);
        if (readCount > 0) {
            const std::size_t written = std::fwrite(buffer.data(), 1, readCount, destination);
            if (written != readCount) {
                std::fclose(source);
                std::fclose(destination);
                outError = Utf8String("Failed while copying glTF into assets.");
                return false;
            }
        }
        if (readCount < buffer.size()) {
            if (std::ferror(source) != 0) {
                std::fclose(source);
                std::fclose(destination);
                outError = Utf8String("Failed while reading glTF source file.");
                return false;
            }
            break;
        }
    }
    std::fclose(source);
    std::fclose(destination);
    return true;
}

void EnsureDirectoryExists(const char* directoryPath) noexcept {
    if (directoryPath == nullptr || directoryPath[0] == '\0') {
        return;
    }
    char path[1024]{};
    std::snprintf(path, sizeof(path), "%s", directoryPath);
    for (std::size_t i = 1; path[i] != '\0'; ++i) {
        if (!IsPathSeparator(path[i])) {
            continue;
        }
        path[i] = '\0';
        if (path[0] != '\0') {
            struct stat st {};
            if (stat(path, &st) != 0) {
                (void)mkdir(path, 0755);
            }
        }
        path[i] = '/';
    }
    struct stat st {};
    if (stat(path, &st) != 0) {
        (void)mkdir(path, 0755);
    }
}

const char* FindFileName(const char* path) noexcept {
    if (path == nullptr) {
        return "";
    }
    const char* last = path;
    for (const char* cursor = path; *cursor != '\0'; ++cursor) {
        if (IsPathSeparator(*cursor) && cursor[1] != '\0') {
            last = cursor + 1;
        }
    }
    return last;
}

Utf8String ExtensionLower(const char* fileName) noexcept {
    const char* dot = nullptr;
    for (const char* cursor = fileName; *cursor != '\0'; ++cursor) {
        if (*cursor == '.') {
            dot = cursor;
        }
    }
    if (dot == nullptr || dot[1] == '\0') {
        return Utf8String(".glb");
    }
    return Utf8String(dot);
}

}  // namespace

bool GltfAssetPathResolver::TryMakeAssetsRelative(const char* path, Utf8String& outRelative) noexcept {
    outRelative.Clear();
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (Utf8String relative = StripRootPrefix(path, SPARK_ASSETS_DIR); !relative.IsEmpty()) {
        outRelative = MoveTemp(relative);
        return true;
    }
    if (Utf8String relative = StripRootPrefix(path, SPARK_BUILD_ASSETS_DIR); !relative.IsEmpty()) {
        outRelative = MoveTemp(relative);
        return true;
    }
    if (std::strncmp(path, "assets/", 7) == 0) {
        outRelative = Utf8String(path + 7);
        return true;
    }
    if (ScenePathResolver::FileExists(path) && std::strchr(path, '/') == nullptr && std::strchr(path, '\\') == nullptr) {
        outRelative = Utf8String(path);
        return true;
    }
    return false;
}

Utf8String GltfAssetPathResolver::SanitizeBaseName(const char* fileName) noexcept {
    Utf8String sanitized{};
    if (fileName == nullptr || fileName[0] == '\0') {
        sanitized = Utf8String("gltf_model");
        return sanitized;
    }
    const char* base = FindFileName(fileName);
    for (const char* cursor = base; *cursor != '\0'; ++cursor) {
        const char ch = *cursor;
        if (ch == '.') {
            break;
        }
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-') {
            char token[2] = {ch, '\0'};
            sanitized.AppendUtf8(token);
        } else if (ch == ' ') {
            sanitized.AppendUtf8("_");
        }
    }
    if (sanitized.IsEmpty()) {
        sanitized = Utf8String("gltf_model");
    }
    return sanitized;
}

bool GltfAssetPathResolver::EnsureInAssetLibrary(
        const char* sourcePath,
        Utf8String& outRelative,
        Utf8String* outError) noexcept {
    outRelative.Clear();
    if (sourcePath == nullptr || sourcePath[0] == '\0') {
        if (outError != nullptr) {
            *outError = Utf8String("Empty glTF path.");
        }
        return false;
    }

    const Utf8String resolved = ScenePathResolver::ResolveReadablePath(sourcePath);
    if (resolved.IsEmpty()) {
        if (outError != nullptr) {
            *outError = Utf8String("glTF file not found.");
        }
        return false;
    }

    if (TryMakeAssetsRelative(resolved.CStr(), outRelative)) {
        return true;
    }

    const char* fileName = FindFileName(resolved.CStr());
    const Utf8String sanitized = SanitizeBaseName(fileName);
    const Utf8String extension = ExtensionLower(fileName);
    Utf8String importedFileName = sanitized;
    importedFileName.AppendUtf8(extension);

    char sourceImportedDir[1024]{};
    char buildImportedDir[1024]{};
    std::snprintf(sourceImportedDir, sizeof(sourceImportedDir), "%s/models/imported", SPARK_ASSETS_DIR);
    std::snprintf(buildImportedDir, sizeof(buildImportedDir), "%s/models/imported", SPARK_BUILD_ASSETS_DIR);
    EnsureDirectoryExists(sourceImportedDir);
    EnsureDirectoryExists(buildImportedDir);

    const Utf8String sourceDestination = ScenePathResolver::SourceAssetPath("models/imported", importedFileName.CStr());
    const Utf8String buildDestination = ScenePathResolver::BuildRuntimePath("models/imported", importedFileName.CStr());

    Utf8String copyError{};
    if (!CopyFileBinary(resolved.CStr(), sourceDestination.CStr(), copyError)) {
        if (outError != nullptr) {
            *outError = MoveTemp(copyError);
        }
        return false;
    }
    (void)CopyFileBinary(resolved.CStr(), buildDestination.CStr(), copyError);

    char destinationRelative[512]{};
    std::snprintf(destinationRelative, sizeof(destinationRelative), "models/imported/%s", importedFileName.CStr());
    outRelative = Utf8String(destinationRelative);
    return true;
}

}  // namespace Spark
