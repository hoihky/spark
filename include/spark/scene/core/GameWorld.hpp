#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObjectQuery.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/assets/GameWorldAssetLoader.hpp"
#include "spark/scene/assets/AssetLoadEvents.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/submit/SkinnedAnimationService.hpp"
#include "spark/scene/submit/SkinnedIkService.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark {

class Font;
class GameObject;
class IEngineContext;

/**
 * Owns GameObjects (ECS entities) and the parent/child graph.
 * Asset loading/caching is delegated to <c>GameWorldAssetCache</c>.
 */
class GameWorld {
public:
    GameWorld();
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;
    GameWorld(GameWorld&&) = delete;
    GameWorld& operator=(GameWorld&&) = delete;
    ~GameWorld();

    [[nodiscard]] GameObject* CreateGameObject();
    void DestroyGameObject(GameObject* object);

    /**
     * child and newParent must belong to this world. newParent may be nullptr (root).
     * Returns false if newParent is not in this world, or if linking would create a cycle.
     */
    bool SetParent(GameObject* child, GameObject* newParent);

    /** Per-frame component simulation (OnUpdate on each GameObject's components). */
    void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path) { return assetCache.LoadMesh(path); }
    [[nodiscard]] GltfAsset LoadGltf(const char* path) { return assetCache.LoadGltf(path); }
    [[nodiscard]] AssetLoadOutcome<GltfAsset> TryLoadGltf(const char* path) { return assetCache.TryLoadGltf(path); }
    [[nodiscard]] GltfSceneDocument LoadGltfScene(const char* path) { return assetCache.LoadGltfScene(path); }
    [[nodiscard]] AssetLoadOutcome<GltfSceneDocument> TryLoadGltfScene(const char* path) {
        return assetCache.TryLoadGltfScene(path);
    }
    void RequestGltf(const char* path) { assetLoader.RequestGltf(path); }
    void RequestSkinnedGltf(const char* path) { assetLoader.RequestSkinnedGltf(path); }
    void OnGltfReady(const char* path, AssetLoadCallback callback) { assetLoader.OnGltfReady(path, callback); }
    void OnSkinnedGltfReady(const char* path, AssetLoadCallback callback) {
        assetLoader.OnSkinnedGltfReady(path, callback);
    }
    void OnTextureReady(const char* path, AssetLoadCallback callback) { assetLoader.OnTextureReady(path, callback); }
    void OnAssetReady(const char* path, AssetLoadJobKind kind, AssetLoadCallback callback) {
        assetLoader.OnAssetReady(path, kind, callback);
    }
    void SetAssetLoadListener(IAssetLoadListener* listener) noexcept { assetLoader.SetAssetLoadListener(listener); }
    [[nodiscard]] IAssetLoadListener* GetAssetLoadListener() const noexcept {
        return assetLoader.GetAssetLoadListener();
    }
    void PumpAssets() { assetLoader.Pump(*this); }
    [[nodiscard]] std::size_t GetPendingAssetLoadCount() const noexcept { return assetLoader.GetPendingJobCount(); }
    [[nodiscard]] std::size_t GetOutstandingAssetLoadCount() const noexcept {
        return assetLoader.GetOutstandingLoadCount();
    }
    [[nodiscard]] bool IsGltfReady(const char* path) const { return assetLoader.IsGltfReady(path); }
    [[nodiscard]] bool IsSkinnedGltfReady(const char* path) const { return assetLoader.IsSkinnedGltfReady(path); }
    [[nodiscard]] AssetLoadState GetAssetLoadState(const char* path, AssetLoadJobKind kind) const {
        return assetLoader.GetState(path, kind);
    }
    [[nodiscard]] Utf8String GetAssetLoadError(const char* path, AssetLoadJobKind kind) const {
        return assetLoader.GetLoadError(path, kind);
    }
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey) { assetCache.RegisterGltf(asset, cacheKey); }
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path) { return assetCache.LoadSkinnedGltf(path); }
    [[nodiscard]] AssetLoadOutcome<SkinnedGltfAsset> TryLoadSkinnedGltf(const char* path) {
        return assetCache.TryLoadSkinnedGltf(path);
    }
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
        assetCache.RegisterSkinnedGltf(asset, cacheKey);
    }
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path) { return assetCache.LoadTexture(path); }
    [[nodiscard]] AssetLoadOutcome<SharedPtr<Texture2D>> TryLoadTexture(const char* path) {
        return assetCache.TryLoadTexture(path);
    }
    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr) {
        return assetCache.RegisterMesh(mesh, cacheKey);
    }
    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr) {
        return assetCache.RegisterTexture(texture, cacheKey);
    }
    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const {
        return assetCache.TryGetMeshByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const {
        return assetCache.TryGetTextureByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const {
        return assetCache.TryGetCachedSkinnedGltf(path, out);
    }
    [[nodiscard]] bool TryGetCachedGltf(const char* path, GltfAsset& out) const {
        return assetCache.TryGetCachedGltf(path, out);
    }
    /** Synchronous load for editor/scripting call sites (blocks the calling thread; no spin-wait). */
    [[nodiscard]] bool AwaitGltf(const char* path, GltfAsset& out);
    /** Synchronous load for editor/scripting call sites (blocks the calling thread; no spin-wait). */
    [[nodiscard]] bool AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out);
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const {
        return assetCache.TryGetAxisAlignedBoundsForKeyOrPath(keyOrPath, outMin, outMax);
    }
    [[nodiscard]] MaterialAsset LoadMaterial(const char* path) { return assetCache.LoadMaterial(path); }
    [[nodiscard]] AssetLoadOutcome<MaterialAsset> TryLoadMaterial(const char* path) {
        return assetCache.TryLoadMaterial(path);
    }
    void RegisterMaterial(const MaterialAsset& material, const char* cacheKey) {
        assetCache.RegisterMaterial(material, cacheKey);
    }
    [[nodiscard]] const MaterialAsset* TryGetMaterialByKeyOrPath(const char* keyOrPath) const {
        return assetCache.TryGetMaterialByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] VfxAsset LoadVfx(const char* path) { return assetCache.LoadVfx(path); }
    [[nodiscard]] AssetLoadOutcome<VfxAsset> TryLoadVfx(const char* path) { return assetCache.TryLoadVfx(path); }
    void RegisterVfx(const VfxAsset& asset, const char* cacheKey) { assetCache.RegisterVfx(asset, cacheKey); }
    [[nodiscard]] const VfxAsset* TryGetVfxByKeyOrPath(const char* keyOrPath) const {
        return assetCache.TryGetVfxByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] VfxSubsystem& GetVfxSubsystem() noexcept { return vfxSubsystem; }
    [[nodiscard]] const VfxSubsystem& GetVfxSubsystem() const noexcept { return vfxSubsystem; }
    [[nodiscard]] SkinnedAnimationService& GetSkinnedAnimationService() noexcept { return skinnedAnimationService; }
    [[nodiscard]] const SkinnedAnimationService& GetSkinnedAnimationService() const noexcept {
        return skinnedAnimationService;
    }
    [[nodiscard]] SkinnedIkService& GetSkinnedIkService() noexcept { return skinnedIkService; }
    [[nodiscard]] const SkinnedIkService& GetSkinnedIkService() const noexcept { return skinnedIkService; }
    void RequestMaterial(const char* path) { assetLoader.RequestMaterial(path); }
    void InvalidateAssetLoadState(const char* path, AssetLoadJobKind kind) {
        assetLoader.InvalidateAssetLoadState(path, kind);
    }
    void OnMaterialReady(const char* path, AssetLoadCallback callback) {
        assetLoader.OnMaterialReady(path, callback);
    }
    [[nodiscard]] bool IsMaterialReady(const char* path) const { return assetLoader.IsMaterialReady(path); }
    [[nodiscard]] bool SaveMaterialAsset(
            const char* path,
            const MaterialAsset& material,
            const char* relativeToDir = nullptr,
            const GameWorldAssetCache* textureKeys = nullptr) {
        return MaterialAssetLoader::TrySaveToFile(path, material, relativeToDir, textureKeys);
    }
    void RetainAsset(CachedAssetKind kind, const char* key) { assetCache.RetainAsset(kind, key); }
    bool ReleaseAsset(CachedAssetKind kind, const char* key) { return assetCache.ReleaseAsset(kind, key); }
    [[nodiscard]] std::uint32_t GetAssetRetainCount(CachedAssetKind kind, const char* key) const {
        return assetCache.GetAssetRetainCount(kind, key);
    }
    [[nodiscard]] bool BuildTextureAtlas(const char* atlasKey, const char* const* textureKeys, std::size_t count) {
        return assetCache.BuildTextureAtlas(atlasKey, textureKeys, count);
    }
    [[nodiscard]] bool TryAutoPackTextureIntoAtlas(const char* atlasGroupKey, const SharedPtr<Texture2D>& texture) {
        return assetCache.TryAutoPackTextureIntoAtlas(atlasGroupKey, texture);
    }
    void FinalizePendingAutoPackAtlas(const char* atlasGroupKey) {
        assetCache.FinalizePendingAutoPackAtlas(atlasGroupKey);
    }

    /** Font used for TextOverlayComponent / screen text when set on SceneRenderParams. */
    void SetUiFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return uiFont; }
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return uiBoldFont; }

    [[nodiscard]] std::size_t GetGameObjectCount() const noexcept { return objects.GetSize(); }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) const {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

    /** Visits objects matching <c>filter</c> (default skips inactive hierarchy). */
    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) {
        ForEachGameObject([&](GameObject* object) {
            if (GameObjectPassesQueryFilter(object, filter)) {
                fn(object);
            }
        });
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) const {
        ForEachGameObject([&](GameObject* object) {
            if (GameObjectPassesQueryFilter(object, filter)) {
                fn(object);
            }
        });
    }

    /** Visits objects that are active in the hierarchy (simulation / render default). */
    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) {
        GameObjectQueryFilter filter;
        filter.includeInactive = false;
        ForEachGameObject(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) const {
        GameObjectQueryFilter filter;
        filter.includeInactive = false;
        ForEachGameObject(Forward<Fn>(fn), filter);
    }

    /**
     * Visits objects that pass <c>filter</c> and have every listed component type.
     * Example: <c>ForEachGameObjectWithComponents&lt;MeshComponent, MaterialComponent&gt;(fn);</c>
     */
    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        ForEachGameObject(
                [&](GameObject* object) {
                    if (GameObjectHasAllComponents<ComponentTypes...>(object)) {
                        fn(object);
                    }
                },
                filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        ForEachGameObject(
                [&](GameObject* object) {
                    if (GameObjectHasAllComponents<ComponentTypes...>(object)) {
                        fn(object);
                    }
                },
                filter);
    }

    /** Visits each matching component instance: <c>fn(object, component)</c>. */
    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        ForEachGameObjectWithComponents<T>([&](GameObject* object) { fn(object, *object->GetComponent<T>()); }, filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        ForEachGameObjectWithComponents<T>([&](GameObject* object) { fn(object, *object->GetComponent<T>()); }, filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(
            const GameObjectQueryFilter& filter = {}) {
        GameObject* found = nullptr;
        ForEachGameObjectWithComponents<ComponentTypes...>([&](GameObject* object) {
            if (found == nullptr) {
                found = object;
            }
        }, filter);
        return found;
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(const GameObjectQueryFilter& filter = {}) const {
        GameObject* found = nullptr;
        ForEachGameObjectWithComponents<ComponentTypes...>([&](GameObject* object) {
            if (found == nullptr) {
                found = object;
            }
        }, filter);
        return found;
    }

    [[nodiscard]] GameObject* FindGameObjectById(std::uint64_t id) const noexcept;
    [[nodiscard]] GameObject* FindGameObjectByName(const char* name, const GameObjectQueryFilter& filter = {}) const;
    [[nodiscard]] GameObject* FindGameObjectWithTag(const char* tag, const GameObjectQueryFilter& filter = {}) const;

    void CollectGameObjects(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const;
    [[nodiscard]] GameObjectQueryResult QueryGameObjects(const GameObjectQueryFilter& filter = {}) const;

    [[nodiscard]] GameWorldAssetCache& GetAssetCache() noexcept { return assetCache; }
    [[nodiscard]] const GameWorldAssetCache& GetAssetCache() const noexcept { return assetCache; }
    [[nodiscard]] GameWorldAssetLoader& GetAssetLoader() noexcept { return assetLoader; }
    [[nodiscard]] const GameWorldAssetLoader& GetAssetLoader() const noexcept { return assetLoader; }

private:
    friend class GameObject;

    void EraseGameObjectStorage(GameObject* pointer);
    static void CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out);

    Array<UniquePtr<GameObject>> objects;
    GameWorldAssetCache assetCache;
    GameWorldAssetLoader assetLoader;
    VfxSubsystem vfxSubsystem{};
    SkinnedAnimationService skinnedAnimationService{};
    SkinnedIkService skinnedIkService{};
    SharedPtr<Font> uiFont{};
    SharedPtr<Font> uiBoldFont{};
    std::uint64_t nextObjectId = 1;
};

}  // namespace Spark
