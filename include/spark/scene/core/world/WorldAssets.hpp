#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/AssetLoadEvents.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/core/world/WorldView.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"

namespace Spark {

class GameWorld;
class GameWorldAssetLoader;
class Mesh;
class Texture2D;
class RenderTexture;
struct GltfAsset;
struct GltfSceneDocument;
class MaterialAsset;
struct SkinnedGltfAsset;

/** Asset cache + async I/O surface for one world. */
class WorldAssets final : public WorldView {
public:
    using WorldView::WorldView;

    [[nodiscard]] GameWorldAssetCache& Cache() noexcept;
    [[nodiscard]] const GameWorldAssetCache& Cache() const noexcept;

    [[nodiscard]] GameWorldAssetLoader& Loader() noexcept;
    [[nodiscard]] const GameWorldAssetLoader& Loader() const noexcept;

    void Pump();
    [[nodiscard]] std::size_t PendingLoadCount() const noexcept;
    [[nodiscard]] std::size_t OutstandingLoadCount() const noexcept;

    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path);
    [[nodiscard]] GltfAsset LoadGltf(const char* path);
    [[nodiscard]] AssetLoadOutcome<GltfAsset> TryLoadGltf(const char* path);
    [[nodiscard]] GltfSceneDocument LoadGltfScene(const char* path);
    [[nodiscard]] AssetLoadOutcome<GltfSceneDocument> TryLoadGltfScene(const char* path);

    void RequestGltf(const char* path);
    void RequestSkinnedGltf(const char* path);
    void RequestMaterial(const char* path);

    void OnGltfReady(const char* path, AssetLoadCallback callback);
    void OnSkinnedGltfReady(const char* path, AssetLoadCallback callback);
    void OnTextureReady(const char* path, AssetLoadCallback callback);
    void OnMaterialReady(const char* path, AssetLoadCallback callback);
    void OnAssetReady(const char* path, AssetLoadJobKind kind, AssetLoadCallback callback);

    void SetLoadListener(IAssetLoadListener* listener) noexcept;
    [[nodiscard]] IAssetLoadListener* GetLoadListener() const noexcept;

    void InvalidateLoadState(const char* path, AssetLoadJobKind kind);

    [[nodiscard]] bool IsGltfReady(const char* path) const;
    [[nodiscard]] bool IsSkinnedGltfReady(const char* path) const;
    [[nodiscard]] bool IsMaterialReady(const char* path) const;
    [[nodiscard]] AssetLoadState GetLoadState(const char* path, AssetLoadJobKind kind) const;
    [[nodiscard]] Utf8String GetLoadError(const char* path, AssetLoadJobKind kind) const;

    void RegisterGltf(const GltfAsset& asset, const char* cacheKey);
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path);
    [[nodiscard]] AssetLoadOutcome<SkinnedGltfAsset> TryLoadSkinnedGltf(const char* path);
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey);

    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path);
    [[nodiscard]] AssetLoadOutcome<SharedPtr<Texture2D>> TryLoadTexture(const char* path);

    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr);
    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr);
    SharedPtr<RenderTexture> RegisterRenderTexture(const SharedPtr<RenderTexture>& texture, const char* cacheKey = nullptr);

    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] SharedPtr<RenderTexture> TryGetRenderTextureByKey(const char* cacheKey) const;

    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const;
    [[nodiscard]] bool TryGetCachedGltf(const char* path, GltfAsset& out) const;

    [[nodiscard]] bool AwaitGltf(const char* path, GltfAsset& out);
    [[nodiscard]] bool AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out);

    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax) const;

    [[nodiscard]] MaterialAsset LoadMaterial(const char* path);
    [[nodiscard]] AssetLoadOutcome<MaterialAsset> TryLoadMaterial(const char* path);
    void RegisterMaterial(const MaterialAsset& material, const char* cacheKey);
    [[nodiscard]] const MaterialAsset* TryGetMaterialByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] bool SaveMaterialAsset(
            const char* path,
            const MaterialAsset& material,
            const char* relativeToDir = nullptr,
            const GameWorldAssetCache* textureKeys = nullptr);

    [[nodiscard]] VfxAsset LoadVfx(const char* path);
    [[nodiscard]] AssetLoadOutcome<VfxAsset> TryLoadVfx(const char* path);
    void RegisterVfx(const VfxAsset& asset, const char* cacheKey);
    [[nodiscard]] const VfxAsset* TryGetVfxByKeyOrPath(const char* keyOrPath) const;

    void RetainAsset(CachedAssetKind kind, const char* key);
    bool ReleaseAsset(CachedAssetKind kind, const char* key);
    [[nodiscard]] std::uint32_t GetAssetRetainCount(CachedAssetKind kind, const char* key) const;

    [[nodiscard]] bool BuildTextureAtlas(const char* atlasKey, const char* const* textureKeys, std::size_t count);
    [[nodiscard]] bool TryAutoPackTextureIntoAtlas(const char* atlasGroupKey, const SharedPtr<Texture2D>& texture);
    void FinalizePendingAutoPackAtlas(const char* atlasGroupKey);
};

}  // namespace Spark
