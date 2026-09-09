#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/assets/GltfAssetPathResolver.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/assets/gltf/GltfSceneImporter.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/assets/gltf/SkinnedGltfMaterialPresenter.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"

namespace Spark {

namespace {

bool ShouldBindGltfMaterials(const GltfAsset& asset) noexcept {
    return static_cast<bool>(asset.mesh);
}

bool ShouldBindGltfMaterials(const SkinnedGltfAsset& asset) noexcept {
    return static_cast<bool>(asset.mesh);
}

[[nodiscard]] Utf8String ResolveGltfLibraryKey(const char* gltfPath) noexcept {
    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return {};
    }
    Utf8String relative{};
    if (GltfAssetPathResolver::TryMakeAssetsRelative(gltfPath, relative)) {
        return relative;
    }
    return Utf8String(gltfPath);
}

[[nodiscard]] std::uint32_t ResolvePrimaryMaterialIndex(const GltfAsset& asset) noexcept {
    if (asset.mesh && !asset.mesh->GetSubmeshes().IsEmpty()) {
        return asset.mesh->GetSubmeshes()[0].materialIndex;
    }
    return 0U;
}

[[nodiscard]] bool ShouldUseMultiMaterialComponent(const GltfAsset& asset) noexcept {
    return static_cast<bool>(asset.mesh) && asset.mesh->GetSubmeshes().GetSize() > 1U;
}

[[nodiscard]] const GltfMaterial& ResolvePrimaryMaterial(
        const GltfAsset& asset,
        const std::uint32_t materialIndex) noexcept {
    if (!asset.materials.IsEmpty()) {
        const std::size_t resolvedIndex =
                materialIndex < asset.materials.GetSize() ? materialIndex : 0U;
        return asset.materials[resolvedIndex];
    }
    return asset.material;
}

void BindMaterials(GameObject& owner, const GltfAsset& asset, const char* gltfLibraryKey) {
    if (!ShouldBindGltfMaterials(asset)) {
        return;
    }
    GameWorld& world = owner.GetWorld();
    const Utf8String libraryKey = ResolveGltfLibraryKey(gltfLibraryKey);
    if (ShouldUseMultiMaterialComponent(asset)) {
        if (MultiMaterialComponent* multi = owner.AddComponent<MultiMaterialComponent>()) {
            multi->BindFromGltfAsset(world, libraryKey.CStr(), asset);
        }
        return;
    }

    const std::uint32_t materialIndex = ResolvePrimaryMaterialIndex(asset);
    const GltfMaterial& primaryMaterial = ResolvePrimaryMaterial(asset, materialIndex);
    if (!primaryMaterial.HasPresentationContent()) {
        return;
    }
    if (MaterialComponent* mat = owner.AddComponent<MaterialComponent>()) {
        ApplyGltfMaterialDesc(*mat, primaryMaterial);
        if (!libraryKey.IsEmpty()) {
            const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(libraryKey.CStr(), materialIndex);
            mat->SetMaterialAsset(world, key.CStr());
        }
    }
}

void BindMaterials(GameObject& owner, const SkinnedGltfAsset& asset, const char* gltfLibraryKey) {
    SkinnedGltfMaterialPresenter presenter{};
    presenter.PresentOn(owner, asset, gltfLibraryKey);
}

}  // namespace

void GltfAssetBinder::ApplyMaterials(GameObject& owner, const GltfAsset& asset, const char* gltfLibraryKey) {
    BindMaterials(owner, asset, gltfLibraryKey);
}

void GltfAssetBinder::ApplyMaterials(GameObject& owner, const SkinnedGltfAsset& asset, const char* gltfLibraryKey) {
    BindMaterials(owner, asset, gltfLibraryKey);
}

void GltfAssetBinder::BindRigidMesh(
        GameObject& owner,
        const GltfAsset& asset,
        const SceneMeshSlot slot,
        const Vector3& albedo,
        const char* gltfLibraryKey) {
    if (!asset.mesh) {
        return;
    }
    if (owner.GetComponent<MeshComponent>() == nullptr) {
        owner.AddComponent<MeshComponent>(asset.mesh, slot, albedo);
    }
    BindMaterials(owner, asset, gltfLibraryKey);
}

void GltfAssetBinder::BindSkinnedMesh(
        GameObject& owner,
        const SkinnedGltfAsset& asset,
        const Vector3& /*albedo*/,
        const char* gltfLibraryKey) {
    if (!asset.mesh) {
        return;
    }
    SkinnedMeshComponent* skinned = owner.GetComponent<SkinnedMeshComponent>();
    if (skinned == nullptr) {
        skinned = owner.AddComponent<SkinnedMeshComponent>(asset.mesh);
    } else {
        skinned->SetMesh(asset.mesh);
    }
    if (asset.skeleton) {
        skinned->SetSkeleton(asset.skeleton);
    }
    BindMaterials(owner, asset, gltfLibraryKey);
}

bool GltfAssetBinder::BindFromPath(
        GameObject& owner,
        const char* gltfPath,
        const SceneMeshSlot slot,
        const Vector3& albedo) {
    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return false;
    }
    const Utf8String resolvedPath = ScenePathResolver::ResolveReadablePath(gltfPath);
    if (resolvedPath.IsEmpty()) {
        return false;
    }
    GameWorld& world = owner.GetWorld();
    const AssetLoadOutcome<GltfSceneDocument> sceneOutcome = world.TryLoadGltfScene(resolvedPath.CStr());
    if (!sceneOutcome.ok) {
        return false;
    }
    const Utf8String libraryKey = ResolveGltfLibraryKey(resolvedPath.CStr());
    return GltfSceneImporter::ImportInto(
            owner, sceneOutcome.value, slot, albedo, libraryKey.CStr());
}

bool GltfAssetBinder::BindFromPathAsChild(
        GameObject& owner,
        const char* gltfPath,
        const SceneMeshSlot slot,
        const Vector3& albedo) {
    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return false;
    }
    GameWorld& world = owner.GetWorld();
    GameObject* visualRoot = world.CreateGameObject();
    if (visualRoot == nullptr) {
        return false;
    }
    visualRoot->GetName() = Utf8String("GltfVisual");
    if (!world.SetParent(visualRoot, &owner)) {
        world.DestroyGameObject(visualRoot);
        return false;
    }
    if (!BindFromPath(*visualRoot, gltfPath, slot, albedo)) {
        world.DestroyGameObject(visualRoot);
        return false;
    }
    return true;
}

}  // namespace Spark
