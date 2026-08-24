#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/assets/gltf/GltfRigidLoader.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/material/MaterialAsset.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
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

void RegisterMaterialTextures(GameWorldAssetCache& cache, const GltfMaterialDesc& material) {
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

void RegisterAllMaterialTextures(GameWorldAssetCache& cache, const GltfAsset& asset) {
    for (std::size_t i = 0; i < asset.materials.GetSize(); ++i) {
        RegisterMaterialTextures(cache, asset.materials[i]);
    }
    if (asset.materials.IsEmpty()) {
        RegisterMaterialTextures(cache, asset.material);
    }
}

void RegisterAllMaterialTextures(GameWorldAssetCache& cache, const SkinnedGltfAsset& asset) {
    for (std::size_t i = 0; i < asset.materials.GetSize(); ++i) {
        RegisterMaterialTextures(cache, asset.materials[i]);
    }
    if (asset.materials.IsEmpty()) {
        RegisterMaterialTextures(cache, asset.material);
    }
}

void SyncLegacyFields(GltfAsset& asset) {
    if (!asset.materials.IsEmpty()) {
        asset.material = asset.materials[0];
    }
    asset.baseColorTexture = asset.material.baseColor;
}

void SyncLegacyFields(SkinnedGltfAsset& asset) {
    if (!asset.materials.IsEmpty()) {
        asset.material = asset.materials[0];
    }
    asset.baseColorTexture = asset.material.baseColor;
}

[[nodiscard]] bool PathEndsWithInsensitive(const char* path, const char* suffix) {
    if (path == nullptr || suffix == nullptr) {
        return false;
    }
    const std::size_t lp = std::strlen(path);
    const std::size_t ls = std::strlen(suffix);
    if (lp < ls) {
        return false;
    }
    for (std::size_t i = 0; i < ls; ++i) {
        char a = path[lp - ls + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsGltfPath(const char* path) {
    return PathEndsWithInsensitive(path, ".glb") || PathEndsWithInsensitive(path, ".gltf");
}

Utf8String MakeGltfMaterialLibraryKey(const char* gltfPath, const std::size_t materialIndex) {
    Utf8String key(gltfPath != nullptr ? gltfPath : "");
    key.AppendUtf8("#material/");
    char indexBuf[16]{};
    std::snprintf(indexBuf, sizeof(indexBuf), "%zu", materialIndex);
    key.AppendUtf8(indexBuf);
    return key;
}

}  // namespace

void GameWorldAssetCache::RegisterGltfMaterialsInLibrary(
        const char* gltfPath,
        const Array<GltfMaterialDesc>& materials) {
    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return;
    }
    for (std::size_t i = 0; i < materials.GetSize(); ++i) {
        MaterialAsset libraryMaterial{};
        libraryMaterial.CopyFromGltfMaterial(materials[i]);
        libraryMaterial.name = MakeGltfMaterialLibraryKey(gltfPath, i);
        RegisterMaterial(libraryMaterial, libraryMaterial.name.CStr());
    }
}

SharedPtr<Mesh> GameWorldAssetCache::LoadMesh(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<Mesh>();
    }
    if (IsGltfPath(path)) {
        return LoadGltf(path).mesh;
    }
    const Utf8String key(path);
    if (const SharedPtr<Mesh>* cached = meshCache.Find(key)) {
        return *cached;
    }
    auto mesh = MakeShared<Mesh>(Utf8String(path));
    if (!Mesh::TryLoadFromObj(path, *mesh)) {
        return SharedPtr<Mesh>();
    }
    meshCache.Add(key, mesh);
    EnsureInitialRetainCount(CachedAssetKind::Mesh, key);
    return mesh;
}

GltfAsset GameWorldAssetCache::LoadGltf(const char* path) {
    return TryLoadGltf(path).value;
}

AssetLoadOutcome<GltfAsset> GameWorldAssetCache::TryLoadGltf(const char* path) {
    AssetLoadOutcome<GltfAsset> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    if (path == nullptr || path[0] == '\0') {
        outcome.errorMessage = Utf8String("Empty glTF path");
        return outcome;
    }
    const Utf8String key(path);
    if (const GltfAsset* cached = gltfCache.Find(key)) {
        outcome.ok = true;
        outcome.value = *cached;
        return outcome;
    }
    GltfRigidLoadResult loaded{};
    if (!GltfRigidLoader{}.LoadFromFile(path, loaded) || !loaded.mesh) {
        outcome.errorMessage = loaded.errorMessage.IsEmpty() ? Utf8String("Failed to load glTF mesh") : loaded.errorMessage;
        std::fprintf(stderr, "Spark: LoadGltf failed: %s\n", outcome.errorMessage.CStr());
        return outcome;
    }
    GltfAsset asset{};
    asset.mesh = loaded.mesh;
    asset.materials = loaded.materials;
    SyncLegacyFields(asset);
    RegisterAllMaterialTextures(*this, asset);
    RegisterGltfMaterialsInLibrary(path, asset.materials);
    gltfCache.Add(key, asset);
    EnsureInitialRetainCount(CachedAssetKind::Gltf, key);
    outcome.ok = true;
    outcome.value = asset;
    return outcome;
}

void GameWorldAssetCache::RegisterGltf(const GltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    GltfAsset stored = asset;
    SyncLegacyFields(stored);
    gltfCache.Add(Utf8String(cacheKey), stored);
    EnsureInitialRetainCount(CachedAssetKind::Gltf, Utf8String(cacheKey));
    RegisterAllMaterialTextures(*this, stored);
    RegisterGltfMaterialsInLibrary(cacheKey, stored.materials);
}

SkinnedGltfAsset GameWorldAssetCache::LoadSkinnedGltf(const char* path) {
    return TryLoadSkinnedGltf(path).value;
}

AssetLoadOutcome<SkinnedGltfAsset> GameWorldAssetCache::TryLoadSkinnedGltf(const char* path) {
    AssetLoadOutcome<SkinnedGltfAsset> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    if (path == nullptr || path[0] == '\0') {
        outcome.errorMessage = Utf8String("Empty skinned glTF path");
        return outcome;
    }
    const Utf8String key(path);
    if (const SkinnedGltfAsset* cached = skinnedGltfCache.Find(key)) {
        outcome.ok = true;
        outcome.value = *cached;
        return outcome;
    }
    SkinnedMesh mesh;
    Skeleton skeleton;
    GltfMaterialDesc material{};
    Array<GltfMaterialDesc> materials;
    std::uint32_t walkClip = 0;
    Quaternion bindUp{};
    float facingYaw = 0.0F;
    Utf8String loadError;
    if (!TryLoadSkinnedCharacterFromGltf(
                path,
                mesh,
                skeleton,
                nullptr,
                &material,
                &walkClip,
                &bindUp,
                &facingYaw,
                &materials,
                &loadError)) {
        outcome.errorMessage = loadError.IsEmpty() ? Utf8String("Failed to load skinned glTF") : loadError;
        std::fprintf(stderr, "Spark: LoadSkinnedGltf failed: %s\n", outcome.errorMessage.CStr());
        return outcome;
    }
    SkinnedGltfAsset asset{};
    asset.mesh = SharedPtr<SkinnedMesh>(new SkinnedMesh(MoveTemp(mesh)));
    asset.skeleton = SharedPtr<Skeleton>(new Skeleton(MoveTemp(skeleton)));
    asset.materials = materials;
    asset.material = material;
    asset.walkClipIndex = walkClip;
    asset.bindUpAlignment = bindUp;
    asset.bindFacingYawOffset = facingYaw;
    SyncLegacyFields(asset);
    RegisterAllMaterialTextures(*this, asset);
    RegisterGltfMaterialsInLibrary(path, asset.materials);
    skinnedGltfCache.Add(key, asset);
    EnsureInitialRetainCount(CachedAssetKind::SkinnedGltf, key);
    outcome.ok = true;
    outcome.value = asset;
    return outcome;
}

void GameWorldAssetCache::RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || !asset.skeleton || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    SkinnedGltfAsset stored = asset;
    SyncLegacyFields(stored);
    skinnedGltfCache.Add(Utf8String(cacheKey), stored);
    EnsureInitialRetainCount(CachedAssetKind::SkinnedGltf, Utf8String(cacheKey));
    RegisterAllMaterialTextures(*this, stored);
    RegisterGltfMaterialsInLibrary(cacheKey, stored.materials);
}

SharedPtr<Texture2D> GameWorldAssetCache::LoadTexture(const char* path) {
    return TryLoadTexture(path).value;
}

AssetLoadOutcome<SharedPtr<Texture2D>> GameWorldAssetCache::TryLoadTexture(const char* path) {
    AssetLoadOutcome<SharedPtr<Texture2D>> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    if (path == nullptr || path[0] == '\0') {
        outcome.errorMessage = Utf8String("Empty texture path");
        return outcome;
    }
    if (!IsRegularFile(path)) {
        outcome.errorMessage = Utf8String("Texture path is not a regular file: ");
        outcome.errorMessage.AppendUtf8(path);
        return outcome;
    }
    const Utf8String key(path);
    if (const SharedPtr<Texture2D>* cached = textureCache.Find(key)) {
        outcome.ok = true;
        outcome.value = *cached;
        return outcome;
    }
    auto tex = MakeShared<Texture2D>(Utf8String(path));
    if (!Texture2D::TryLoadFromFile(path, *tex)) {
        outcome.errorMessage = Utf8String("Failed to decode texture: ");
        outcome.errorMessage.AppendUtf8(path);
        std::fprintf(stderr, "Spark: LoadTexture failed: %s\n", outcome.errorMessage.CStr());
        return outcome;
    }
    textureCache.Add(key, tex);
    EnsureInitialRetainCount(CachedAssetKind::Texture, key);
    outcome.ok = true;
    outcome.value = tex;
    return outcome;
}

SharedPtr<Texture2D> GameWorldAssetCache::TryLoadTextureIfFileExists(const char* path) {
    if (!IsRegularFile(path)) {
        return SharedPtr<Texture2D>();
    }
    return TryLoadTexture(path).value;
}

SharedPtr<Mesh> GameWorldAssetCache::RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey) {
    if (!mesh) {
        return SharedPtr<Mesh>();
    }
    if (cacheKey != nullptr && cacheKey[0] != '\0') {
        meshCache.Add(Utf8String(cacheKey), mesh);
        EnsureInitialRetainCount(CachedAssetKind::Mesh, Utf8String(cacheKey));
    }
    return mesh;
}

SharedPtr<Texture2D> GameWorldAssetCache::RegisterTexture(
        const SharedPtr<Texture2D>& texture,
        const char* cacheKey) {
    if (!texture) {
        return SharedPtr<Texture2D>();
    }
    if (cacheKey != nullptr && cacheKey[0] != '\0') {
        const Utf8String key(cacheKey);
        textureCache.Add(key, texture);
        textureKeyByPointer.Add(texture.Get(), key);
        const Utf8String& displayName = texture->GetName();
        if (!displayName.IsEmpty() && displayName != key) {
            textureCache.Add(displayName, texture);
        }
        EnsureInitialRetainCount(CachedAssetKind::Texture, key);
    }
    return texture;
}

SharedPtr<Mesh> GameWorldAssetCache::TryGetMeshByKeyOrPath(const char* keyOrPath) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return SharedPtr<Mesh>();
    }
    const Utf8String key(keyOrPath);
    if (const GltfAsset* g = gltfCache.Find(key)) {
        return g->mesh;
    }
    if (const SharedPtr<Mesh>* m = meshCache.Find(key)) {
        return *m;
    }
    return SharedPtr<Mesh>();
}

SharedPtr<Texture2D> GameWorldAssetCache::TryGetTextureByKeyOrPath(const char* keyOrPath) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return SharedPtr<Texture2D>();
    }
    const Utf8String key(keyOrPath);
    const SkinnedGltfAsset* sk = skinnedGltfCache.Find(key);
    const GltfAsset* gl = gltfCache.Find(key);
    if (sk != nullptr && sk->baseColorTexture) {
        return sk->baseColorTexture;
    }
    if (gl != nullptr && gl->baseColorTexture) {
        return gl->baseColorTexture;
    }
    if (const SharedPtr<Texture2D>* t = textureCache.Find(key)) {
        return *t;
    }
    if (sk != nullptr) {
        return sk->baseColorTexture;
    }
    if (gl != nullptr) {
        return gl->baseColorTexture;
    }
    return SharedPtr<Texture2D>();
}

Utf8String GameWorldAssetCache::TryFindTextureKey(const Texture2D* texture) const {
    if (texture == nullptr) {
        return {};
    }
    if (const Utf8String* key = textureKeyByPointer.Find(texture)) {
        return *key;
    }
    return texture->GetName();
}

bool GameWorldAssetCache::TryGetCachedGltf(const char* path, GltfAsset& out) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (const GltfAsset* g = gltfCache.Find(Utf8String(path))) {
        out = *g;
        return static_cast<bool>(out.mesh);
    }
    return false;
}

bool GameWorldAssetCache::TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (const SkinnedGltfAsset* s = skinnedGltfCache.Find(Utf8String(path))) {
        out = *s;
        return static_cast<bool>(out.mesh && out.skeleton);
    }
    return false;
}

bool GameWorldAssetCache::TryGetAxisAlignedBoundsForKeyOrPath(
        const char* keyOrPath,
        Vector3& outMin,
        Vector3& outMax) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return false;
    }
    const Utf8String key(keyOrPath);
    if (const GltfAsset* g = gltfCache.Find(key)) {
        if (g->mesh && g->mesh->TryComputeAxisAlignedBounds(outMin, outMax)) {
            return true;
        }
    }
    if (const SharedPtr<Mesh>* m = meshCache.Find(key)) {
        if (*m && (*m)->TryComputeAxisAlignedBounds(outMin, outMax)) {
            return true;
        }
    }
    if (const SkinnedGltfAsset* sk = skinnedGltfCache.Find(key)) {
        if (!sk->mesh) {
            return false;
        }
        const Array<SkinnedMesh::Vertex>& sv = sk->mesh->GetVertices();
        if (sv.IsEmpty()) {
            return false;
        }
        outMin = sv[0].position;
        outMax = sv[0].position;
        for (std::size_t vi = 1; vi < sv.GetSize(); ++vi) {
            const Vector3& p = sv[vi].position;
            outMin.x = std::min(outMin.x, p.x);
            outMin.y = std::min(outMin.y, p.y);
            outMin.z = std::min(outMin.z, p.z);
            outMax.x = std::max(outMax.x, p.x);
            outMax.y = std::max(outMax.y, p.y);
            outMax.z = std::max(outMax.z, p.z);
        }
        return true;
    }
    return false;
}

MaterialAsset GameWorldAssetCache::LoadMaterial(const char* path) {
    return TryLoadMaterial(path).value;
}

AssetLoadOutcome<MaterialAsset> GameWorldAssetCache::TryLoadMaterial(const char* path) {
    AssetLoadOutcome<MaterialAsset> outcome{};
    if (path != nullptr) {
        outcome.path = Utf8String(path);
    }
    if (path == nullptr || path[0] == '\0') {
        outcome.errorMessage = Utf8String("Empty material path");
        return outcome;
    }
    const Utf8String key(path);
    if (const MaterialAsset* cached = materialCache.Find(key)) {
        outcome.ok = true;
        outcome.value = *cached;
        return outcome;
    }
    outcome = MaterialAssetLoader::TryLoadFromFile(path, *this);
    if (outcome.ok) {
        materialCache.Add(key, outcome.value);
        EnsureInitialRetainCount(CachedAssetKind::Material, key);
    } else if (outcome.errorMessage.IsEmpty()) {
        outcome.errorMessage = Utf8String("Failed to load material asset");
        std::fprintf(stderr, "Spark: LoadMaterial failed: %s\n", path);
    }
    return outcome;
}

void GameWorldAssetCache::RegisterMaterial(const MaterialAsset& material, const char* cacheKey) {
    if (cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    materialCache.Add(Utf8String(cacheKey), material);
    EnsureInitialRetainCount(CachedAssetKind::Material, Utf8String(cacheKey));
    RegisterMaterialTextures(*this, material);
}

const MaterialAsset* GameWorldAssetCache::TryGetMaterialByKeyOrPath(const char* keyOrPath) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return nullptr;
    }
    return materialCache.Find(Utf8String(keyOrPath));
}

void GameWorldAssetCache::BumpRetainCount(const CachedAssetKind kind, const Utf8String& key) {
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* map = nullptr;
    switch (kind) {
        case CachedAssetKind::Mesh:
            map = &meshRetainCounts;
            break;
        case CachedAssetKind::Texture:
            map = &textureRetainCounts;
            break;
        case CachedAssetKind::Material:
            map = &materialRetainCounts;
            break;
        case CachedAssetKind::Gltf:
            map = &gltfRetainCounts;
            break;
        case CachedAssetKind::SkinnedGltf:
            map = &skinnedGltfRetainCounts;
            break;
    }
    if (map == nullptr) {
        return;
    }
    if (std::uint32_t* count = map->Find(key)) {
        ++(*count);
    } else {
        map->Add(key, 1U);
    }
}

void GameWorldAssetCache::EnsureInitialRetainCount(const CachedAssetKind kind, const Utf8String& key) {
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* map = nullptr;
    switch (kind) {
        case CachedAssetKind::Mesh:
            map = &meshRetainCounts;
            break;
        case CachedAssetKind::Texture:
            map = &textureRetainCounts;
            break;
        case CachedAssetKind::Material:
            map = &materialRetainCounts;
            break;
        case CachedAssetKind::Gltf:
            map = &gltfRetainCounts;
            break;
        case CachedAssetKind::SkinnedGltf:
            map = &skinnedGltfRetainCounts;
            break;
    }
    if (map != nullptr && map->Find(key) == nullptr) {
        map->Add(key, 1U);
    }
}

std::uint32_t GameWorldAssetCache::GetRetainCount(const CachedAssetKind kind, const Utf8String& key) const {
    const HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* map = nullptr;
    switch (kind) {
        case CachedAssetKind::Mesh:
            map = &meshRetainCounts;
            break;
        case CachedAssetKind::Texture:
            map = &textureRetainCounts;
            break;
        case CachedAssetKind::Material:
            map = &materialRetainCounts;
            break;
        case CachedAssetKind::Gltf:
            map = &gltfRetainCounts;
            break;
        case CachedAssetKind::SkinnedGltf:
            map = &skinnedGltfRetainCounts;
            break;
    }
    if (map == nullptr) {
        return 0U;
    }
    if (const std::uint32_t* count = map->Find(key)) {
        return *count;
    }
    return 0U;
}

bool GameWorldAssetCache::EvictIfUnretained(const CachedAssetKind kind, const Utf8String& key) {
    if (GetRetainCount(kind, key) != 0U) {
        return false;
    }
    switch (kind) {
        case CachedAssetKind::Mesh:
            meshCache.Remove(key);
            break;
        case CachedAssetKind::Texture:
            if (const SharedPtr<Texture2D>* tex = textureCache.Find(key)) {
                const Utf8String displayName = (*tex)->GetName();
                if (!displayName.IsEmpty() && displayName != key) {
                    textureCache.Remove(displayName);
                }
                textureKeyByPointer.Remove(tex->Get());
            }
            textureCache.Remove(key);
            break;
        case CachedAssetKind::Material:
            materialCache.Remove(key);
            break;
        case CachedAssetKind::Gltf:
            gltfCache.Remove(key);
            break;
        case CachedAssetKind::SkinnedGltf:
            skinnedGltfCache.Remove(key);
            break;
    }
    if (HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* map = [&]() -> HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* {
        switch (kind) {
            case CachedAssetKind::Mesh:
                return &meshRetainCounts;
            case CachedAssetKind::Texture:
                return &textureRetainCounts;
            case CachedAssetKind::Material:
                return &materialRetainCounts;
            case CachedAssetKind::Gltf:
                return &gltfRetainCounts;
            case CachedAssetKind::SkinnedGltf:
                return &skinnedGltfRetainCounts;
        }
        return nullptr;
    }()) {
        map->Remove(key);
    }
    return true;
}

void GameWorldAssetCache::RetainAsset(const CachedAssetKind kind, const char* key) {
    if (key == nullptr || key[0] == '\0') {
        return;
    }
    BumpRetainCount(kind, Utf8String(key));
}

bool GameWorldAssetCache::ReleaseAsset(const CachedAssetKind kind, const char* key) {
    if (key == nullptr || key[0] == '\0') {
        return false;
    }
    const Utf8String cacheKey(key);
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher>* map = nullptr;
    switch (kind) {
        case CachedAssetKind::Mesh:
            map = &meshRetainCounts;
            break;
        case CachedAssetKind::Texture:
            map = &textureRetainCounts;
            break;
        case CachedAssetKind::Material:
            map = &materialRetainCounts;
            break;
        case CachedAssetKind::Gltf:
            map = &gltfRetainCounts;
            break;
        case CachedAssetKind::SkinnedGltf:
            map = &skinnedGltfRetainCounts;
            break;
    }
    if (map == nullptr) {
        return false;
    }
    if (std::uint32_t* count = map->Find(cacheKey)) {
        if (*count > 0U) {
            --(*count);
        }
        if (*count == 0U) {
            return EvictIfUnretained(kind, cacheKey);
        }
    }
    return false;
}

std::uint32_t GameWorldAssetCache::GetAssetRetainCount(const CachedAssetKind kind, const char* key) const {
    if (key == nullptr || key[0] == '\0') {
        return 0U;
    }
    return GetRetainCount(kind, Utf8String(key));
}

bool GameWorldAssetCache::TryAutoPackTextureIntoAtlas(
        const char* atlasGroupKey,
        const SharedPtr<Texture2D>& texture) {
    if (atlasGroupKey == nullptr || atlasGroupKey[0] == '\0' || !texture || texture->HasAtlasBinding()) {
        return false;
    }
    if (texture->GetWidth() > 512U || texture->GetHeight() > 512U) {
        return false;
    }
    if (!autoPackAtlasBuilder) {
        autoPackAtlasBuilder = MakeUnique<SceneTextureAtlas>();
    }
    if (!autoPackAtlasBuilder->TryAdd(texture)) {
        Utf8String atlasName = Utf8String("atlas/");
        atlasName.AppendUtf8(atlasGroupKey);
        if (SharedPtr<Texture2D> atlas = autoPackAtlasBuilder->Finalize(atlasName)) {
            RegisterTexture(atlas, atlasName.CStr());
            finalizedAtlases.Add(Utf8String(atlasGroupKey), atlas);
        }
        autoPackAtlasBuilder = MakeUnique<SceneTextureAtlas>();
        if (!autoPackAtlasBuilder->TryAdd(texture)) {
            return false;
        }
    }
    return true;
}

void GameWorldAssetCache::FinalizePendingAutoPackAtlas(const char* atlasGroupKey) {
    if (atlasGroupKey == nullptr || atlasGroupKey[0] == '\0') {
        return;
    }
    if (!autoPackAtlasBuilder || autoPackAtlasBuilder->IsEmpty()) {
        return;
    }
    Utf8String atlasName = Utf8String("atlas/");
    atlasName.AppendUtf8(atlasGroupKey);
    if (SharedPtr<Texture2D> atlas = autoPackAtlasBuilder->Finalize(atlasName)) {
        RegisterTexture(atlas, atlasName.CStr());
        finalizedAtlases.Add(Utf8String(atlasGroupKey), atlas);
    }
    autoPackAtlasBuilder.Reset();
}

bool GameWorldAssetCache::BuildTextureAtlas(
        const char* atlasKey,
        const char* const* textureKeys,
        const std::size_t count) {
    if (atlasKey == nullptr || atlasKey[0] == '\0' || textureKeys == nullptr || count == 0U) {
        return false;
    }
    SceneTextureAtlas builder{};
    for (std::size_t i = 0; i < count; ++i) {
        const char* textureKey = textureKeys[i];
        if (textureKey == nullptr || textureKey[0] == '\0') {
            continue;
        }
        SharedPtr<Texture2D> tex = TryGetTextureByKeyOrPath(textureKey);
        if (!tex) {
            tex = LoadTexture(textureKey);
        }
        if (!tex) {
            continue;
        }
        (void)builder.TryAdd(tex);
    }
    if (builder.IsEmpty()) {
        return false;
    }
    Utf8String atlasName = Utf8String("atlas/");
    atlasName.AppendUtf8(atlasKey);
    SharedPtr<Texture2D> atlas = builder.Finalize(atlasName);
    if (!atlas) {
        return false;
    }
    RegisterTexture(atlas, atlasKey);
    finalizedAtlases.Add(Utf8String(atlasKey), atlas);
    return true;
}

}  // namespace Spark
