#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/GameWorldAssetCache.hpp"

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
    GameWorld() = default;
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
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey) { assetCache.RegisterGltf(asset, cacheKey); }
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path) { return assetCache.LoadSkinnedGltf(path); }
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
        assetCache.RegisterSkinnedGltf(asset, cacheKey);
    }
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path) { return assetCache.LoadTexture(path); }
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
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const {
        return assetCache.TryGetAxisAlignedBoundsForKeyOrPath(keyOrPath, outMin, outMax);
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

    [[nodiscard]] GameWorldAssetCache& GetAssetCache() noexcept { return assetCache; }
    [[nodiscard]] const GameWorldAssetCache& GetAssetCache() const noexcept { return assetCache; }

private:
    friend class GameObject;

    void EraseGameObjectStorage(GameObject* pointer);
    static void CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out);

    Array<UniquePtr<GameObject>> objects;
    GameWorldAssetCache assetCache;
    SharedPtr<Font> uiFont{};
    SharedPtr<Font> uiBoldFont{};
    std::uint64_t nextObjectId = 1;
};

}  // namespace Spark
