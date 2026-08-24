#include "spark/scene/material/MaterialAssetLoader.hpp"

#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"

#include <cstdio>
#include <cstring>
#include <functional>
#include <sys/stat.h>

namespace Spark {

namespace {

bool IsRegularFile(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    struct stat st {};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

Utf8String ParentDirectory(const char* filePath) {
    if (filePath == nullptr) {
        return {};
    }
    const char* last = nullptr;
    for (const char* p = filePath; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            last = p;
        }
    }
    if (last == nullptr) {
        return {};
    }
    Utf8String out;
    for (const char* q = filePath; q < last; ++q) {
        const char unit[2] = {*q, '\0'};
        out.AppendUtf8(unit);
    }
    return out;
}

Utf8String ResolveTexturePath(const char* materialPath, const char* relativePath) {
    if (relativePath == nullptr || relativePath[0] == '\0') {
        return {};
    }
    if (relativePath[0] == '/' || std::strstr(relativePath, "://") != nullptr) {
        return Utf8String(relativePath);
    }
    Utf8String out = ParentDirectory(materialPath);
    if (!out.IsEmpty()) {
        out.AppendUtf8("/");
    }
    out.AppendUtf8(relativePath);
    return out;
}


void RegisterMaterialTextures(GameWorldAssetCache& cache, const MaterialAsset& material) {
    auto registerOne = [&](const SharedPtr<Texture2D>& tex) {
        if (tex) {
            cache.RegisterTexture(tex, tex->GetName().CStr());
        }
    };
    registerOne(material.baseColor);
    registerOne(material.normalMap);
    registerOne(material.metallicRoughness);
    registerOne(material.emissiveMap);
}

bool PopulateAssetFromSlot(
        const char* materialPath,
        const MaterialSlotSnapshot::Data& slot,
        MaterialAsset& out,
        const std::function<bool(const Utf8String& relativePath, SharedPtr<Texture2D>& outTexture)>& bindTexture) {
    if (!bindTexture(slot.baseColorPath, out.baseColor) || !bindTexture(slot.normalPath, out.normalMap) ||
        !bindTexture(slot.metallicRoughnessPath, out.metallicRoughness) ||
        !bindTexture(slot.emissivePath, out.emissiveMap)) {
        return false;
    }
    if (materialPath != nullptr) {
        out.name = Utf8String(materialPath);
    }
    out.tint = slot.tint;
    out.metallic = slot.metallic;
    out.roughness = slot.roughness;
    out.metallicFactor = slot.metallicFactor;
    out.roughnessFactor = slot.roughnessFactor;
    out.occlusionStrength = slot.occlusionStrength;
    out.emissiveColor = slot.emissiveColor;
    out.emissiveIntensity = slot.emissiveIntensity;
    out.emissiveFactor = slot.emissiveFactor;
    out.shadingModel = slot.shadingModel;
    out.doubleSided = slot.doubleSided;
    out.opacity = slot.opacity;
    out.alphaCutoff = slot.alphaCutoff;
    return true;
}

bool FileExists(const char* path) noexcept {
    return IsRegularFile(path);
}

Utf8String TryExistingPath(const char* path) {
    if (FileExists(path)) {
        return Utf8String(path);
    }
    return {};
}

Utf8String JoinRootRelative(const char* root, const char* relative) {
    if (root == nullptr || relative == nullptr || relative[0] == '\0') {
        return {};
    }
    while (relative[0] == '/') {
        ++relative;
    }
    char buf[1024]{};
    const int written = std::snprintf(buf, sizeof(buf), "%s/%s", root, relative);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buf)) {
        return {};
    }
    return TryExistingPath(buf);
}

bool TryReadSparkMatBodyFromDisk(const char* diskPath, MaterialSlotSnapshot::Data& slot) {
    if (diskPath == nullptr || diskPath[0] == '\0') {
        return false;
    }
    std::FILE* file = std::fopen(diskPath, "rb");
    if (file == nullptr) {
        return false;
    }
    char header[32]{};
    if (std::fscanf(file, "%31s", header) != 1 || std::strcmp(header, "sparkmat_v1") != 0) {
        std::fclose(file);
        return false;
    }
    int ch = 0;
    do {
        ch = std::fgetc(file);
    } while (ch != '\n' && ch != EOF);
    char line[4096]{};
    if (std::fgets(line, static_cast<int>(sizeof(line)), file) == nullptr) {
        std::fclose(file);
        return false;
    }
    std::fclose(file);
    const char* cursor = line;
    return MaterialSlotSnapshot::TryParseSlotV1(cursor, slot);
}

bool TryReadSparkMatBody(const char* path, MaterialSlotSnapshot::Data& slot) {
    const Utf8String resolved = MaterialAssetLoader::ResolveReadablePath(path);
    if (resolved.IsEmpty()) {
        return false;
    }
    return TryReadSparkMatBodyFromDisk(resolved.CStr(), slot);
}

}  // namespace

Utf8String MaterialAssetLoader::ResolveReadablePath(const char* keyOrPath) {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return {};
    }
    if (Utf8String direct = TryExistingPath(keyOrPath); !direct.IsEmpty()) {
        return direct;
    }
    if (Utf8String build = JoinRootRelative(SPARK_BUILD_ASSETS_DIR, keyOrPath); !build.IsEmpty()) {
        return build;
    }
    if (Utf8String source = JoinRootRelative(SPARK_ASSETS_DIR, keyOrPath); !source.IsEmpty()) {
        return source;
    }
    return {};
}

AssetLoadOutcome<MaterialAsset> MaterialAssetLoader::TryLoadFromFile(
        const char* path,
        GameWorldAssetCache& cache) {
    AssetLoadOutcome<MaterialAsset> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    outcome.ok = TryLoadFromFile(path, outcome.value, cache);
    if (!outcome.ok && outcome.errorMessage.IsEmpty()) {
        outcome.errorMessage = Utf8String("Failed to load material asset: ");
        outcome.errorMessage.AppendUtf8(path != nullptr ? path : "");
    }
    return outcome;
}

bool MaterialAssetLoader::TryLoadFromFile(const char* path, MaterialAsset& out, GameWorldAssetCache& cache) {
    MaterialAssetFilePayload payload{};
    if (!TryDecodeFromFile(path, payload)) {
        return false;
    }
    const Utf8String resolved = ResolveReadablePath(path);
    ResolveMaterialTextures(cache, resolved.CStr(), payload.slot, payload.asset);
    out = MoveTemp(payload.asset);
    RegisterMaterialTextures(cache, out);
    return true;
}

void MaterialAssetLoader::ResolveMaterialTextures(
        GameWorldAssetCache& cache,
        const char* materialDiskPath,
        const MaterialSlotSnapshot::Data& slot,
        MaterialAsset& inOutAsset) {
    auto resolveOne = [&](const Utf8String& keyOrPath, SharedPtr<Texture2D>& outTexture) {
        if (keyOrPath.IsEmpty()) {
            outTexture = SharedPtr<Texture2D>();
            return;
        }
        if (SharedPtr<Texture2D> cached = cache.TryGetTextureByKeyOrPath(keyOrPath.CStr())) {
            outTexture = cached;
            return;
        }
        const Utf8String besideMaterial = ResolveTexturePath(materialDiskPath, keyOrPath.CStr());
        if (SharedPtr<Texture2D> cached = cache.TryGetTextureByKeyOrPath(besideMaterial.CStr())) {
            outTexture = cached;
            return;
        }
        if (SharedPtr<Texture2D> loaded = cache.TryLoadTextureIfFileExists(besideMaterial.CStr())) {
            outTexture = loaded;
            return;
        }
        if (Utf8String buildPath = JoinRootRelative(SPARK_BUILD_ASSETS_DIR, keyOrPath.CStr()); !buildPath.IsEmpty()) {
            if (SharedPtr<Texture2D> loaded = cache.TryLoadTextureIfFileExists(buildPath.CStr())) {
                outTexture = loaded;
                return;
            }
        }
        if (Utf8String sourcePath = JoinRootRelative(SPARK_ASSETS_DIR, keyOrPath.CStr()); !sourcePath.IsEmpty()) {
            if (SharedPtr<Texture2D> loaded = cache.TryLoadTextureIfFileExists(sourcePath.CStr())) {
                outTexture = loaded;
                return;
            }
        }
        if (SharedPtr<Texture2D> loaded = cache.TryLoadTextureIfFileExists(keyOrPath.CStr())) {
            outTexture = loaded;
            return;
        }
        outTexture = SharedPtr<Texture2D>();
    };

    resolveOne(slot.baseColorPath, inOutAsset.baseColor);
    resolveOne(slot.normalPath, inOutAsset.normalMap);
    resolveOne(slot.metallicRoughnessPath, inOutAsset.metallicRoughness);
    resolveOne(slot.emissivePath, inOutAsset.emissiveMap);
}

bool MaterialAssetLoader::TryDecodeFromFile(const char* path, MaterialAssetFilePayload& out) {
    out = MaterialAssetFilePayload{};
    const Utf8String resolved = ResolveReadablePath(path);
    if (resolved.IsEmpty()) {
        return false;
    }
    if (!TryReadSparkMatBodyFromDisk(resolved.CStr(), out.slot)) {
        return false;
    }
    auto bindDeferred = [&](const Utf8String&, SharedPtr<Texture2D>& outTexture) -> bool {
        outTexture = SharedPtr<Texture2D>();
        return true;
    };
    return PopulateAssetFromSlot(resolved.CStr(), out.slot, out.asset, bindDeferred);
}

bool MaterialAssetLoader::TrySaveToFile(
        const char* path,
        const MaterialAsset& material,
        const char* relativeToDir,
        const GameWorldAssetCache* textureKeys) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    const char* relativeBase = relativeToDir != nullptr ? relativeToDir : ParentDirectory(path).CStr();
    MaterialSlotSnapshot::Data slot{};
    MaterialSlotSnapshot::CaptureFromAsset(material, slot, relativeBase, textureKeys);

    Utf8String body;
    MaterialSlotSnapshot::AppendSlotV1(slot, body);

    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "sparkmat_v1\n%s\n", body.CStr());
    std::fclose(file);
    return true;
}

}  // namespace Spark
