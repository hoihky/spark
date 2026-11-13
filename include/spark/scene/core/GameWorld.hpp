#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObjectQuery.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/AssetLoadEvents.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/assets/GameWorldAssetLoader.hpp"
#include "spark/scene/core/EntityId.hpp"
#include "spark/scene/core/GameWorldAssetStore.hpp"
#include "spark/scene/core/GameWorldEntityGraph.hpp"
#include "spark/scene/core/GameWorldPresentation.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"

namespace Spark {

class Font;
class GameObject;
class IEngineContext;
class PhysicsSubsystem;
class WorldAssets;
class WorldEntities;
class WorldPhysics;
class WorldRendering;
class WorldSimulation;
class WorldQuery;

/**
 * Facade composing entity graph, assets, and presentation services.
 * Flat methods remain for compatibility; prefer <c>Entities()</c>, <c>Assets()</c>, <c>Simulation()</c>, …
 */
class GameWorld {
public:
    GameWorld();
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;
    GameWorld(GameWorld&&) = delete;
    GameWorld& operator=(GameWorld&&) = delete;
    ~GameWorld();

    // --- Entity graph (delegates to <c>entityGraph_</c>) ---
    [[nodiscard]] GameObject* CreateGameObject() { return entityGraph_.Create(); }
    void DestroyGameObject(GameObject* object) { entityGraph_.Destroy(object); }
    void DestroyGameObjectAtEndOfFrame(GameObject* object) { entityGraph_.DestroyAtEndOfFrame(object); }
    void DestroyGameObjectImmediate(GameObject* object) { entityGraph_.DestroyImmediate(object); }
    void FlushPendingDestroyGameObjects() { entityGraph_.FlushPendingDestroys(); }
    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent) {
        return entityGraph_.SetParent(child, newParent);
    }

    [[nodiscard]] std::size_t GetGameObjectCount() const noexcept { return entityGraph_.GetCount(); }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) {
        entityGraph_.ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) const {
        entityGraph_.ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) {
        entityGraph_.ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) const {
        entityGraph_.ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) {
        entityGraph_.ForEachActive(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) const {
        entityGraph_.ForEachActive(Forward<Fn>(fn));
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        entityGraph_.ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        entityGraph_.ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        entityGraph_.ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        entityGraph_.ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(const GameObjectQueryFilter& filter = {}) {
        return entityGraph_.FindFirstWithComponents<ComponentTypes...>(filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindFirstWithComponents<ComponentTypes...>(filter);
    }

    [[nodiscard]] GameObject* FindGameObjectById(std::uint64_t id) const noexcept { return entityGraph_.FindById(id); }
    [[nodiscard]] GameObject* TryGetGameObject(EntityId id) const noexcept { return entityGraph_.TryGet(id); }
    [[nodiscard]] bool IsGameObjectAlive(const GameObject* object) const noexcept { return entityGraph_.IsAlive(object); }
    [[nodiscard]] GameObject* FindGameObjectByName(const char* name, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindByName(name, filter);
    }
    [[nodiscard]] GameObject* FindGameObjectWithTag(const char* tag, const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindWithTag(tag, filter);
    }
    void CollectGameObjects(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
        entityGraph_.Collect(out, filter);
    }
    [[nodiscard]] GameObjectQueryResult QueryGameObjects(const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.Query(filter);
    }

    void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

    // --- Assets (delegates to <c>WorldAssets</c> via compat wrappers) ---
    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path) { return assetStore_.Cache().LoadMesh(path); }
    [[nodiscard]] GltfAsset LoadGltf(const char* path) { return assetStore_.Cache().LoadGltf(path); }
    [[nodiscard]] AssetLoadOutcome<GltfAsset> TryLoadGltf(const char* path) {
        return assetStore_.Cache().TryLoadGltf(path);
    }
    [[nodiscard]] GltfSceneDocument LoadGltfScene(const char* path) { return assetStore_.Cache().LoadGltfScene(path); }
    [[nodiscard]] AssetLoadOutcome<GltfSceneDocument> TryLoadGltfScene(const char* path) {
        return assetStore_.Cache().TryLoadGltfScene(path);
    }
    void RequestGltf(const char* path) { assetStore_.Loader().RequestGltf(path); }
    void RequestSkinnedGltf(const char* path) { assetStore_.Loader().RequestSkinnedGltf(path); }
    void OnGltfReady(const char* path, AssetLoadCallback callback) {
        assetStore_.Loader().OnGltfReady(path, callback);
    }
    void OnSkinnedGltfReady(const char* path, AssetLoadCallback callback) {
        assetStore_.Loader().OnSkinnedGltfReady(path, callback);
    }
    void OnTextureReady(const char* path, AssetLoadCallback callback) {
        assetStore_.Loader().OnTextureReady(path, callback);
    }
    void OnAssetReady(const char* path, AssetLoadJobKind kind, AssetLoadCallback callback) {
        assetStore_.Loader().OnAssetReady(path, kind, callback);
    }
    void SetAssetLoadListener(IAssetLoadListener* listener) noexcept {
        assetStore_.Loader().SetAssetLoadListener(listener);
    }
    [[nodiscard]] IAssetLoadListener* GetAssetLoadListener() const noexcept {
        return assetStore_.Loader().GetAssetLoadListener();
    }
    void PumpAssets() { assetStore_.Loader().Pump(*this); }
    [[nodiscard]] std::size_t GetPendingAssetLoadCount() const noexcept {
        return assetStore_.Loader().GetPendingJobCount();
    }
    [[nodiscard]] std::size_t GetOutstandingAssetLoadCount() const noexcept {
        return assetStore_.Loader().GetOutstandingLoadCount();
    }
    [[nodiscard]] bool IsGltfReady(const char* path) const { return assetStore_.Loader().IsGltfReady(path); }
    [[nodiscard]] bool IsSkinnedGltfReady(const char* path) const {
        return assetStore_.Loader().IsSkinnedGltfReady(path);
    }
    [[nodiscard]] AssetLoadState GetAssetLoadState(const char* path, AssetLoadJobKind kind) const {
        return assetStore_.Loader().GetState(path, kind);
    }
    [[nodiscard]] Utf8String GetAssetLoadError(const char* path, AssetLoadJobKind kind) const {
        return assetStore_.Loader().GetLoadError(path, kind);
    }
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey) {
        assetStore_.Cache().RegisterGltf(asset, cacheKey);
    }
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path) {
        return assetStore_.Cache().LoadSkinnedGltf(path);
    }
    [[nodiscard]] AssetLoadOutcome<SkinnedGltfAsset> TryLoadSkinnedGltf(const char* path) {
        return assetStore_.Cache().TryLoadSkinnedGltf(path);
    }
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
        assetStore_.Cache().RegisterSkinnedGltf(asset, cacheKey);
    }
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path) { return assetStore_.Cache().LoadTexture(path); }
    [[nodiscard]] AssetLoadOutcome<SharedPtr<Texture2D>> TryLoadTexture(const char* path) {
        return assetStore_.Cache().TryLoadTexture(path);
    }
    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr) {
        return assetStore_.Cache().RegisterMesh(mesh, cacheKey);
    }
    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr) {
        return assetStore_.Cache().RegisterTexture(texture, cacheKey);
    }
    SharedPtr<RenderTexture> RegisterRenderTexture(
            const SharedPtr<RenderTexture>& texture,
            const char* cacheKey = nullptr) {
        return assetStore_.Cache().RegisterRenderTexture(texture, cacheKey);
    }
    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const {
        return assetStore_.Cache().TryGetMeshByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const {
        return assetStore_.Cache().TryGetTextureByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] SharedPtr<RenderTexture> TryGetRenderTextureByKey(const char* cacheKey) const {
        return assetStore_.Cache().TryGetRenderTextureByKey(cacheKey);
    }
    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const {
        return assetStore_.Cache().TryGetCachedSkinnedGltf(path, out);
    }
    [[nodiscard]] bool TryGetCachedGltf(const char* path, GltfAsset& out) const {
        return assetStore_.Cache().TryGetCachedGltf(path, out);
    }
    [[nodiscard]] bool AwaitGltf(const char* path, GltfAsset& out);
    [[nodiscard]] bool AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out);
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const {
        return assetStore_.Cache().TryGetAxisAlignedBoundsForKeyOrPath(keyOrPath, outMin, outMax);
    }
    [[nodiscard]] MaterialAsset LoadMaterial(const char* path) { return assetStore_.Cache().LoadMaterial(path); }
    [[nodiscard]] AssetLoadOutcome<MaterialAsset> TryLoadMaterial(const char* path) {
        return assetStore_.Cache().TryLoadMaterial(path);
    }
    void RegisterMaterial(const MaterialAsset& material, const char* cacheKey) {
        assetStore_.Cache().RegisterMaterial(material, cacheKey);
    }
    [[nodiscard]] const MaterialAsset* TryGetMaterialByKeyOrPath(const char* keyOrPath) const {
        return assetStore_.Cache().TryGetMaterialByKeyOrPath(keyOrPath);
    }
    [[nodiscard]] VfxAsset LoadVfx(const char* path) { return assetStore_.Cache().LoadVfx(path); }
    [[nodiscard]] AssetLoadOutcome<VfxAsset> TryLoadVfx(const char* path) {
        return assetStore_.Cache().TryLoadVfx(path);
    }
    void RegisterVfx(const VfxAsset& asset, const char* cacheKey) {
        assetStore_.Cache().RegisterVfx(asset, cacheKey);
    }
    [[nodiscard]] const VfxAsset* TryGetVfxByKeyOrPath(const char* keyOrPath) const {
        return assetStore_.Cache().TryGetVfxByKeyOrPath(keyOrPath);
    }
    void RequestMaterial(const char* path) { assetStore_.Loader().RequestMaterial(path); }
    void InvalidateAssetLoadState(const char* path, AssetLoadJobKind kind) {
        assetStore_.Loader().InvalidateAssetLoadState(path, kind);
    }
    void OnMaterialReady(const char* path, AssetLoadCallback callback) {
        assetStore_.Loader().OnMaterialReady(path, callback);
    }
    [[nodiscard]] bool IsMaterialReady(const char* path) const { return assetStore_.Loader().IsMaterialReady(path); }
    [[nodiscard]] bool SaveMaterialAsset(
            const char* path,
            const MaterialAsset& material,
            const char* relativeToDir = nullptr,
            const GameWorldAssetCache* textureKeys = nullptr);
    void RetainAsset(CachedAssetKind kind, const char* key) { assetStore_.Cache().RetainAsset(kind, key); }
    bool ReleaseAsset(CachedAssetKind kind, const char* key) { return assetStore_.Cache().ReleaseAsset(kind, key); }
    [[nodiscard]] std::uint32_t GetAssetRetainCount(CachedAssetKind kind, const char* key) const {
        return assetStore_.Cache().GetAssetRetainCount(kind, key);
    }
    [[nodiscard]] bool BuildTextureAtlas(const char* atlasKey, const char* const* textureKeys, std::size_t count) {
        return assetStore_.Cache().BuildTextureAtlas(atlasKey, textureKeys, count);
    }
    [[nodiscard]] bool TryAutoPackTextureIntoAtlas(const char* atlasGroupKey, const SharedPtr<Texture2D>& texture) {
        return assetStore_.Cache().TryAutoPackTextureIntoAtlas(atlasGroupKey, texture);
    }
    void FinalizePendingAutoPackAtlas(const char* atlasGroupKey) {
        assetStore_.Cache().FinalizePendingAutoPackAtlas(atlasGroupKey);
    }

    [[nodiscard]] GameWorldAssetCache& GetAssetCache() noexcept { return assetStore_.Cache(); }
    [[nodiscard]] const GameWorldAssetCache& GetAssetCache() const noexcept { return assetStore_.Cache(); }
    [[nodiscard]] GameWorldAssetLoader& GetAssetLoader() noexcept { return assetStore_.Loader(); }
    [[nodiscard]] const GameWorldAssetLoader& GetAssetLoader() const noexcept { return assetStore_.Loader(); }

    // --- Presentation ---
    [[nodiscard]] VfxSubsystem& GetVfxSubsystem() noexcept { return presentation_.Vfx(); }
    [[nodiscard]] const VfxSubsystem& GetVfxSubsystem() const noexcept { return presentation_.Vfx(); }
    [[nodiscard]] WindSubsystem& GetWindSubsystem() noexcept { return presentation_.Wind(); }
    [[nodiscard]] const WindSubsystem& GetWindSubsystem() const noexcept { return presentation_.Wind(); }
    [[nodiscard]] SkinnedAnimationService& GetSkinnedAnimationService() noexcept {
        return presentation_.SkinnedAnimation();
    }
    [[nodiscard]] const SkinnedAnimationService& GetSkinnedAnimationService() const noexcept {
        return presentation_.SkinnedAnimation();
    }
    [[nodiscard]] SkinnedIkService& GetSkinnedIkService() noexcept { return presentation_.SkinnedIk(); }
    [[nodiscard]] const SkinnedIkService& GetSkinnedIkService() const noexcept { return presentation_.SkinnedIk(); }
    void SetUiFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return presentation_.GetUiFont(); }
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return presentation_.GetUiBoldFont(); }

    // --- Grouped views ---
    [[nodiscard]] WorldEntities Entities() noexcept;
    [[nodiscard]] WorldEntities Entities() const noexcept;
    [[nodiscard]] WorldAssets Assets() noexcept;
    [[nodiscard]] WorldAssets Assets() const noexcept;
    [[nodiscard]] WorldSimulation Simulation() noexcept;
    [[nodiscard]] WorldSimulation Simulation() const noexcept;
    [[nodiscard]] WorldRendering Rendering() noexcept;
    [[nodiscard]] WorldRendering Rendering() const noexcept;
    [[nodiscard]] WorldPhysics Physics(PhysicsSubsystem& physics) noexcept;
    [[nodiscard]] WorldPhysics Physics(PhysicsSubsystem& physics) const noexcept;

    [[nodiscard]] WorldQuery QueryObjects() noexcept;
    [[nodiscard]] WorldQuery QueryObjects() const noexcept;

    [[nodiscard]] GameWorldEntityGraph& GetEntityGraph() noexcept { return entityGraph_; }
    [[nodiscard]] const GameWorldEntityGraph& GetEntityGraph() const noexcept { return entityGraph_; }

private:
    friend class GameObject;

    GameWorldEntityGraph entityGraph_;
    GameWorldAssetStore assetStore_;
    GameWorldPresentation presentation_;
};

}  // namespace Spark

#include "spark/scene/core/GameWorldFacets.hpp"
