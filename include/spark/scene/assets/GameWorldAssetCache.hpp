#pragma once

#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/assets/gltf/GltfSceneDocument.hpp"
#include "spark/scene/texture/SceneTextureAtlas.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

/** Result of loading a glTF asset (mesh + full PBR material table). */
struct GltfAsset {
    SharedPtr<Mesh> mesh;
    /** @deprecated Use <c>materials[0]</c> or <c>material.baseColor</c>. */
    SharedPtr<Texture2D> baseColorTexture;
    /** @deprecated Use <c>materials</c>; kept for existing call sites. */
    GltfMaterialDesc material;
    /** All materials in the glTF file, indexed by primitive material index. */
    Array<GltfMaterialDesc> materials;
    /** KHR_materials_variants names (empty when absent). */
    Array<Utf8String> materialVariantNames;
};

/** Skinned glTF: mesh + skeleton + animations + full PBR material table. */
struct SkinnedGltfAsset {
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
    /** @deprecated Use <c>materials[0]</c> or <c>material.baseColor</c>. */
    SharedPtr<Texture2D> baseColorTexture;
    /** @deprecated Use <c>materials</c>. */
    GltfMaterialDesc material;
    Array<GltfMaterialDesc> materials;
    Array<Utf8String> materialVariantNames;
    std::uint32_t walkClipIndex = 0;
    Quaternion bindUpAlignment{Quaternion::Identity};
    float bindFacingYawOffset = 0.0F;
};

namespace Detail {

struct Utf8StringHasher {
    [[nodiscard]] std::size_t operator()(const Utf8String& s) const noexcept {
        const char* p = s.CStr();
        std::size_t h = 5381;
        while (*p != '\0') {
            h = ((h << 5) + h) + static_cast<unsigned char>(*p++);
        }
        return h;
    }
};

struct TexturePointerHasher {
    [[nodiscard]] std::size_t operator()(const Texture2D* ptr) const noexcept {
        return reinterpret_cast<std::size_t>(ptr);
    }
};

}  // namespace Detail

/**
 * Path-keyed mesh / glTF / texture caches (Single Responsibility: asset I/O, not entity graph).
 * Owned by <c>GameWorld</c> and shared with async loaders via register APIs.
 */
class GameWorldAssetCache {
public:
    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path);
    [[nodiscard]] GltfAsset LoadGltf(const char* path);
    [[nodiscard]] AssetLoadOutcome<GltfAsset> TryLoadGltf(const char* path);
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey);
    [[nodiscard]] GltfSceneDocument LoadGltfScene(const char* path);
    [[nodiscard]] AssetLoadOutcome<GltfSceneDocument> TryLoadGltfScene(const char* path);
    [[nodiscard]] bool TryGetCachedGltfScene(const char* path, GltfSceneDocument& out) const;
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path);
    [[nodiscard]] AssetLoadOutcome<SkinnedGltfAsset> TryLoadSkinnedGltf(const char* path);
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey);
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path);
    [[nodiscard]] AssetLoadOutcome<SharedPtr<Texture2D>> TryLoadTexture(const char* path);
    /** Loads only when <c>path</c> is a readable regular file; otherwise returns null without logging. */
    [[nodiscard]] SharedPtr<Texture2D> TryLoadTextureIfFileExists(const char* path);
    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr);
    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr);
    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] Utf8String TryFindTextureKey(const Texture2D* texture) const;
    [[nodiscard]] bool TryGetCachedGltf(const char* path, GltfAsset& out) const;
    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const;
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const;

    [[nodiscard]] MaterialAsset LoadMaterial(const char* path);
    [[nodiscard]] AssetLoadOutcome<MaterialAsset> TryLoadMaterial(const char* path);
    void RegisterMaterial(const MaterialAsset& material, const char* cacheKey);
    [[nodiscard]] const MaterialAsset* TryGetMaterialByKeyOrPath(const char* keyOrPath) const;

    [[nodiscard]] VfxAsset LoadVfx(const char* path);
    [[nodiscard]] AssetLoadOutcome<VfxAsset> TryLoadVfx(const char* path);
    void RegisterVfx(const VfxAsset& asset, const char* cacheKey);
    [[nodiscard]] const VfxAsset* TryGetVfxByKeyOrPath(const char* keyOrPath) const;

    void RetainAsset(CachedAssetKind kind, const char* key);
    /** Decrements retain count; evicts the cache entry when it reaches zero. Returns true when evicted. */
    bool ReleaseAsset(CachedAssetKind kind, const char* key);
    [[nodiscard]] std::uint32_t GetAssetRetainCount(CachedAssetKind kind, const char* key) const;

    /**
     * Packs textures into a named atlas (registered under <c>atlasKey</c>).
     * Each source texture receives an atlas binding for scene UV remapping.
     */
    [[nodiscard]] bool BuildTextureAtlas(const char* atlasKey, const char* const* textureKeys, std::size_t count);
    /** Packs a single texture into an existing or new atlas group when it fits. */
    [[nodiscard]] bool TryAutoPackTextureIntoAtlas(const char* atlasGroupKey, const SharedPtr<Texture2D>& texture);
    /** Finalizes a partially filled auto-pack atlas so textures are uploadable before the layer cap is hit. */
    void FinalizePendingAutoPackAtlas(const char* atlasGroupKey);

private:
    void BumpRetainCount(CachedAssetKind kind, const Utf8String& key);
    void EnsureInitialRetainCount(CachedAssetKind kind, const Utf8String& key);
    bool EvictIfUnretained(CachedAssetKind kind, const Utf8String& key);
    [[nodiscard]] std::uint32_t GetRetainCount(CachedAssetKind kind, const Utf8String& key) const;

    void RegisterGltfMaterialsInLibrary(
            const char* gltfPath,
            const Array<GltfMaterialDesc>& materials,
            const GltfMaterialDesc* legacyMaterial = nullptr);

    HashMap<Utf8String, SharedPtr<Mesh>, Detail::Utf8StringHasher> meshCache;
    HashMap<Utf8String, GltfAsset, Detail::Utf8StringHasher> gltfCache;
    HashMap<Utf8String, GltfSceneDocument, Detail::Utf8StringHasher> gltfSceneCache;
    HashMap<Utf8String, SkinnedGltfAsset, Detail::Utf8StringHasher> skinnedGltfCache;
    HashMap<Utf8String, SharedPtr<Texture2D>, Detail::Utf8StringHasher> textureCache;
    HashMap<const Texture2D*, Utf8String, Detail::TexturePointerHasher> textureKeyByPointer;
    HashMap<Utf8String, MaterialAsset, Detail::Utf8StringHasher> materialCache;
    HashMap<Utf8String, VfxAsset, Detail::Utf8StringHasher> vfxCache;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> meshRetainCounts;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> textureRetainCounts;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> materialRetainCounts;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> gltfRetainCounts;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> gltfSceneRetainCounts;
    HashMap<Utf8String, std::uint32_t, Detail::Utf8StringHasher> skinnedGltfRetainCounts;
    HashMap<Utf8String, SharedPtr<Texture2D>, Detail::Utf8StringHasher> finalizedAtlases;
    UniquePtr<SceneTextureAtlas> autoPackAtlasBuilder;
};

}  // namespace Spark
