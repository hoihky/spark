#include "spark/scene/core/world/WorldAssets.hpp"

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

GameWorldAssetCache& WorldAssets::Cache() noexcept {
    return world_.AssetStore().Cache();
}

const GameWorldAssetCache& WorldAssets::Cache() const noexcept {
    return world_.AssetStore().Cache();
}

GameWorldAssetLoader& WorldAssets::Loader() noexcept {
    return world_.AssetStore().Loader();
}

const GameWorldAssetLoader& WorldAssets::Loader() const noexcept {
    return world_.AssetStore().Loader();
}

void WorldAssets::Pump() {
    world_.AssetStore().Loader().Pump(world_);
}

std::size_t WorldAssets::PendingLoadCount() const noexcept {
    return world_.AssetStore().Loader().GetPendingJobCount();
}

std::size_t WorldAssets::OutstandingLoadCount() const noexcept {
    return world_.AssetStore().Loader().GetOutstandingLoadCount();
}

SharedPtr<Mesh> WorldAssets::LoadMesh(const char* path) {
    return Cache().LoadMesh(path);
}

GltfAsset WorldAssets::LoadGltf(const char* path) {
    return Cache().LoadGltf(path);
}

AssetLoadOutcome<GltfAsset> WorldAssets::TryLoadGltf(const char* path) {
    return Cache().TryLoadGltf(path);
}

GltfSceneDocument WorldAssets::LoadGltfScene(const char* path) {
    return Cache().LoadGltfScene(path);
}

AssetLoadOutcome<GltfSceneDocument> WorldAssets::TryLoadGltfScene(const char* path) {
    return Cache().TryLoadGltfScene(path);
}

void WorldAssets::RequestGltf(const char* path) {
    Loader().RequestGltf(path);
}

void WorldAssets::RequestSkinnedGltf(const char* path) {
    Loader().RequestSkinnedGltf(path);
}

void WorldAssets::RequestMaterial(const char* path) {
    Loader().RequestMaterial(path);
}

void WorldAssets::OnGltfReady(const char* path, AssetLoadCallback callback) {
    Loader().OnGltfReady(path, callback);
}

void WorldAssets::OnSkinnedGltfReady(const char* path, AssetLoadCallback callback) {
    Loader().OnSkinnedGltfReady(path, callback);
}

void WorldAssets::OnTextureReady(const char* path, AssetLoadCallback callback) {
    Loader().OnTextureReady(path, callback);
}

void WorldAssets::OnMaterialReady(const char* path, AssetLoadCallback callback) {
    Loader().OnMaterialReady(path, callback);
}

void WorldAssets::OnAssetReady(const char* path, AssetLoadJobKind kind, AssetLoadCallback callback) {
    Loader().OnAssetReady(path, kind, callback);
}

void WorldAssets::SetLoadListener(IAssetLoadListener* listener) noexcept {
    Loader().SetAssetLoadListener(listener);
}

IAssetLoadListener* WorldAssets::GetLoadListener() const noexcept {
    return Loader().GetAssetLoadListener();
}

void WorldAssets::InvalidateLoadState(const char* path, AssetLoadJobKind kind) {
    Loader().InvalidateAssetLoadState(path, kind);
}

bool WorldAssets::IsGltfReady(const char* path) const {
    return Loader().IsGltfReady(path);
}

bool WorldAssets::IsSkinnedGltfReady(const char* path) const {
    return Loader().IsSkinnedGltfReady(path);
}

bool WorldAssets::IsMaterialReady(const char* path) const {
    return Loader().IsMaterialReady(path);
}

AssetLoadState WorldAssets::GetLoadState(const char* path, AssetLoadJobKind kind) const {
    return Loader().GetState(path, kind);
}

Utf8String WorldAssets::GetLoadError(const char* path, AssetLoadJobKind kind) const {
    return Loader().GetLoadError(path, kind);
}

void WorldAssets::RegisterGltf(const GltfAsset& asset, const char* cacheKey) {
    Cache().RegisterGltf(asset, cacheKey);
}

SkinnedGltfAsset WorldAssets::LoadSkinnedGltf(const char* path) {
    return Cache().LoadSkinnedGltf(path);
}

AssetLoadOutcome<SkinnedGltfAsset> WorldAssets::TryLoadSkinnedGltf(const char* path) {
    return Cache().TryLoadSkinnedGltf(path);
}

void WorldAssets::RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
    Cache().RegisterSkinnedGltf(asset, cacheKey);
}

SharedPtr<Texture2D> WorldAssets::LoadTexture(const char* path) {
    return Cache().LoadTexture(path);
}

AssetLoadOutcome<SharedPtr<Texture2D>> WorldAssets::TryLoadTexture(const char* path) {
    return Cache().TryLoadTexture(path);
}

SharedPtr<Mesh> WorldAssets::RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey) {
    return Cache().RegisterMesh(mesh, cacheKey);
}

SharedPtr<Texture2D> WorldAssets::RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey) {
    return Cache().RegisterTexture(texture, cacheKey);
}

SharedPtr<RenderTexture> WorldAssets::RegisterRenderTexture(
        const SharedPtr<RenderTexture>& texture,
        const char* cacheKey) {
    return Cache().RegisterRenderTexture(texture, cacheKey);
}

SharedPtr<Mesh> WorldAssets::TryGetMeshByKeyOrPath(const char* keyOrPath) const {
    return Cache().TryGetMeshByKeyOrPath(keyOrPath);
}

SharedPtr<Texture2D> WorldAssets::TryGetTextureByKeyOrPath(const char* keyOrPath) const {
    return Cache().TryGetTextureByKeyOrPath(keyOrPath);
}

SharedPtr<RenderTexture> WorldAssets::TryGetRenderTextureByKey(const char* cacheKey) const {
    return Cache().TryGetRenderTextureByKey(cacheKey);
}

bool WorldAssets::TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const {
    return Cache().TryGetCachedSkinnedGltf(path, out);
}

bool WorldAssets::TryGetCachedGltf(const char* path, GltfAsset& out) const {
    return Cache().TryGetCachedGltf(path, out);
}

bool WorldAssets::AwaitGltf(const char* path, GltfAsset& out) {
    out = GltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedGltf(path, out) && out.mesh) {
        return true;
    }
    const AssetLoadOutcome<GltfAsset> loaded = TryLoadGltf(path);
    out = loaded.value;
    return loaded.ok && static_cast<bool>(out.mesh);
}

bool WorldAssets::AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out) {
    out = SkinnedGltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedSkinnedGltf(path, out) && out.mesh && out.skeleton) {
        return true;
    }
    const AssetLoadOutcome<SkinnedGltfAsset> loaded = TryLoadSkinnedGltf(path);
    out = loaded.value;
    return loaded.ok && static_cast<bool>(out.mesh) && static_cast<bool>(out.skeleton);
}

bool WorldAssets::TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
        const {
    return Cache().TryGetAxisAlignedBoundsForKeyOrPath(keyOrPath, outMin, outMax);
}

MaterialAsset WorldAssets::LoadMaterial(const char* path) {
    return Cache().LoadMaterial(path);
}

AssetLoadOutcome<MaterialAsset> WorldAssets::TryLoadMaterial(const char* path) {
    return Cache().TryLoadMaterial(path);
}

void WorldAssets::RegisterMaterial(const MaterialAsset& material, const char* cacheKey) {
    Cache().RegisterMaterial(material, cacheKey);
}

const MaterialAsset* WorldAssets::TryGetMaterialByKeyOrPath(const char* keyOrPath) const {
    return Cache().TryGetMaterialByKeyOrPath(keyOrPath);
}

bool WorldAssets::SaveMaterialAsset(
        const char* path,
        const MaterialAsset& material,
        const char* relativeToDir,
        const GameWorldAssetCache* textureKeys) {
    return MaterialAssetLoader::TrySaveToFile(path, material, relativeToDir, textureKeys);
}

VfxAsset WorldAssets::LoadVfx(const char* path) {
    return Cache().LoadVfx(path);
}

AssetLoadOutcome<VfxAsset> WorldAssets::TryLoadVfx(const char* path) {
    return Cache().TryLoadVfx(path);
}

void WorldAssets::RegisterVfx(const VfxAsset& asset, const char* cacheKey) {
    Cache().RegisterVfx(asset, cacheKey);
}

const VfxAsset* WorldAssets::TryGetVfxByKeyOrPath(const char* keyOrPath) const {
    return Cache().TryGetVfxByKeyOrPath(keyOrPath);
}

void WorldAssets::RetainAsset(CachedAssetKind kind, const char* key) {
    Cache().RetainAsset(kind, key);
}

bool WorldAssets::ReleaseAsset(CachedAssetKind kind, const char* key) {
    return Cache().ReleaseAsset(kind, key);
}

std::uint32_t WorldAssets::GetAssetRetainCount(CachedAssetKind kind, const char* key) const {
    return Cache().GetAssetRetainCount(kind, key);
}

bool WorldAssets::BuildTextureAtlas(const char* atlasKey, const char* const* textureKeys, std::size_t count) {
    return Cache().BuildTextureAtlas(atlasKey, textureKeys, count);
}

bool WorldAssets::TryAutoPackTextureIntoAtlas(const char* atlasGroupKey, const SharedPtr<Texture2D>& texture) {
    return Cache().TryAutoPackTextureIntoAtlas(atlasGroupKey, texture);
}

void WorldAssets::FinalizePendingAutoPackAtlas(const char* atlasGroupKey) {
    Cache().FinalizePendingAutoPackAtlas(atlasGroupKey);
}

}  // namespace Spark
