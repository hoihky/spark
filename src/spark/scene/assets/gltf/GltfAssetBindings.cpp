#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"

namespace Spark {

namespace {

bool AssetHasMaterials(const GltfAsset& asset) noexcept {
    if (!asset.materials.IsEmpty()) {
        for (std::size_t i = 0; i < asset.materials.GetSize(); ++i) {
            if (asset.materials[i].HasAnyTexture()) {
                return true;
            }
        }
    }
    return asset.material.HasAnyTexture();
}

bool AssetHasMaterials(const SkinnedGltfAsset& asset) noexcept {
    if (!asset.materials.IsEmpty()) {
        for (std::size_t i = 0; i < asset.materials.GetSize(); ++i) {
            if (asset.materials[i].HasAnyTexture()) {
                return true;
            }
        }
    }
    return asset.material.HasAnyTexture();
}

void BindMaterials(GameObject& owner, const GltfAsset& asset, const char* gltfLibraryKey) {
    if (!asset.mesh || !AssetHasMaterials(asset)) {
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
    if (!asset.mesh || !AssetHasMaterials(asset)) {
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
    if (owner.GetComponent<SkinnedMeshComponent>() == nullptr) {
        owner.AddComponent<SkinnedMeshComponent>(asset.mesh);
    }
    BindMaterials(owner, asset, gltfLibraryKey);
}

}  // namespace Spark
