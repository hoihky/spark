#include "spark/scene/GameWorldAssetCache.hpp"
#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/GltfRigidLoader.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"

#include <algorithm>
#include <cstring>

namespace Spark {

namespace {

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

}  // namespace

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
    return mesh;
}

GltfAsset GameWorldAssetCache::LoadGltf(const char* path) {
    GltfAsset empty{};
    if (path == nullptr || path[0] == '\0') {
        return empty;
    }
    const Utf8String key(path);
    if (const GltfAsset* cached = gltfCache.Find(key)) {
        return *cached;
    }
    GltfRigidLoadResult loaded{};
    if (!GltfRigidLoader{}.LoadFromFile(path, loaded) || !loaded.mesh) {
        return empty;
    }
    GltfAsset asset{};
    asset.mesh = loaded.mesh;
    asset.materials = loaded.materials;
    SyncLegacyFields(asset);
    RegisterAllMaterialTextures(*this, asset);
    gltfCache.Add(key, asset);
    return asset;
}

void GameWorldAssetCache::RegisterGltf(const GltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    GltfAsset stored = asset;
    SyncLegacyFields(stored);
    gltfCache.Add(Utf8String(cacheKey), stored);
    RegisterAllMaterialTextures(*this, stored);
}

SkinnedGltfAsset GameWorldAssetCache::LoadSkinnedGltf(const char* path) {
    SkinnedGltfAsset empty{};
    if (path == nullptr || path[0] == '\0') {
        return empty;
    }
    const Utf8String key(path);
    if (const SkinnedGltfAsset* cached = skinnedGltfCache.Find(key)) {
        return *cached;
    }
    SkinnedMesh mesh;
    Skeleton skeleton;
    GltfMaterialDesc material{};
    Array<GltfMaterialDesc> materials;
    std::uint32_t walkClip = 0;
    Quaternion bindUp{};
    float facingYaw = 0.0F;
    if (!TryLoadSkinnedCharacterFromGltf(
                path, mesh, skeleton, nullptr, &material, &walkClip, &bindUp, &facingYaw, &materials)) {
        return empty;
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
    skinnedGltfCache.Add(key, asset);
    return asset;
}

void GameWorldAssetCache::RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || !asset.skeleton || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    SkinnedGltfAsset stored = asset;
    SyncLegacyFields(stored);
    skinnedGltfCache.Add(Utf8String(cacheKey), stored);
    RegisterAllMaterialTextures(*this, stored);
}

SharedPtr<Texture2D> GameWorldAssetCache::LoadTexture(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<Texture2D>();
    }
    const Utf8String key(path);
    if (const SharedPtr<Texture2D>* cached = textureCache.Find(key)) {
        return *cached;
    }
    auto tex = MakeShared<Texture2D>(Utf8String(path));
    if (!Texture2D::TryLoadFromFile(path, *tex)) {
        return SharedPtr<Texture2D>();
    }
    textureCache.Add(key, tex);
    return tex;
}

SharedPtr<Mesh> GameWorldAssetCache::RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey) {
    if (!mesh) {
        return SharedPtr<Mesh>();
    }
    if (cacheKey != nullptr && cacheKey[0] != '\0') {
        meshCache.Add(Utf8String(cacheKey), mesh);
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
        textureCache.Add(Utf8String(cacheKey), texture);
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

}  // namespace Spark
