#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/assets/gltf/GltfContentClassifier.hpp"
#include "spark/scene/assets/gltf/GltfSceneImporter.hpp"
#include "spark/scene/core/GameWorld.hpp"
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

void BindMaterials(GameObject& owner, const GltfAsset& asset, const char* gltfLibraryKey) {
    if (!ShouldBindGltfMaterials(asset)) {
        return;
    }
    GameWorld& world = owner.GetWorld();
    const bool multiSubmesh = asset.mesh->GetSubmeshes().GetSize() > 1 ||
                              (!asset.materials.IsEmpty() && asset.materials.GetSize() > 1);
    if (multiSubmesh) {
        if (MultiMaterialComponent* multi = owner.AddComponent<MultiMaterialComponent>()) {
            multi->BindFromGltfAsset(world, gltfLibraryKey, asset);
        }
        return;
    }
    if (MaterialComponent* mat = owner.AddComponent<MaterialComponent>()) {
        if (gltfLibraryKey != nullptr && gltfLibraryKey[0] != '\0') {
            const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(gltfLibraryKey, 0);
            mat->SetMaterialAsset(world, key.CStr());
            return;
        }
        if (!asset.materials.IsEmpty()) {
            ApplyGltfMaterialDesc(*mat, asset.materials[0]);
        } else {
            ApplyGltfMaterialDesc(*mat, asset.material);
        }
    }
}

void BindMaterials(GameObject& owner, const SkinnedGltfAsset& asset, const char* gltfLibraryKey) {
    if (!ShouldBindGltfMaterials(asset)) {
        return;
    }
    GameWorld& world = owner.GetWorld();
    const bool multiSubmesh = asset.mesh->GetSubmeshes().GetSize() > 1 ||
                              (!asset.materials.IsEmpty() && asset.materials.GetSize() > 1);
    if (multiSubmesh) {
        GltfAsset rigidView{};
        rigidView.material = asset.material;
        rigidView.materials = asset.materials;
        if (MultiMaterialComponent* multi = owner.AddComponent<MultiMaterialComponent>()) {
            multi->BindFromGltfAsset(world, gltfLibraryKey, rigidView);
        }
        return;
    }
    if (MaterialComponent* mat = owner.AddComponent<MaterialComponent>()) {
        if (gltfLibraryKey != nullptr && gltfLibraryKey[0] != '\0') {
            const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(gltfLibraryKey, 0);
            mat->SetMaterialAsset(world, key.CStr());
            return;
        }
        if (!asset.materials.IsEmpty()) {
            ApplyGltfMaterialDesc(*mat, asset.materials[0]);
        } else {
            ApplyGltfMaterialDesc(*mat, asset.material);
        }
    }
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
    GameWorld& world = owner.GetWorld();
    const GltfContentClassifier::ProbeResult probe = GltfContentClassifier::ProbeFile(gltfPath);
    if (!probe.parseOk) {
        return false;
    }
    if (probe.kind == GltfContentKind::Skinned) {
        SkinnedGltfAsset skinned = world.LoadSkinnedGltf(gltfPath);
        if (!skinned.mesh) {
            return false;
        }
        BindSkinnedMesh(owner, skinned, albedo, gltfPath);
        return true;
    }
    const AssetLoadOutcome<GltfSceneDocument> sceneOutcome = world.TryLoadGltfScene(gltfPath);
    if (!sceneOutcome.ok) {
        return false;
    }
    return GltfSceneImporter::ImportInto(owner, sceneOutcome.value, slot, albedo, gltfPath);
}

}  // namespace Spark
