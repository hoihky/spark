#include "spark/scene/GltfAssetBindings.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/GltfMaterial.hpp"

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

void BindMaterials(GameObject& owner, const GltfAsset& asset) {
    if (!asset.mesh || !AssetHasMaterials(asset)) {
        return;
    }
    const bool multiSubmesh = asset.mesh->GetSubmeshes().GetSize() > 1 ||
                              (!asset.materials.IsEmpty() && asset.materials.GetSize() > 1);
    if (multiSubmesh) {
        if (MultiMaterialComponent* multi = owner.AddComponent<MultiMaterialComponent>()) {
            multi->PopulateFromGltfAsset(asset);
        }
        return;
    }
    if (MaterialComponent* mat = owner.AddComponent<MaterialComponent>()) {
        if (!asset.materials.IsEmpty()) {
            ApplyGltfMaterialDesc(*mat, asset.materials[0]);
        } else {
            ApplyGltfMaterialDesc(*mat, asset.material);
        }
    }
}

void BindMaterials(GameObject& owner, const SkinnedGltfAsset& asset) {
    if (!asset.mesh || !AssetHasMaterials(asset)) {
        return;
    }
    const bool multiSubmesh = asset.mesh->GetSubmeshes().GetSize() > 1 ||
                              (!asset.materials.IsEmpty() && asset.materials.GetSize() > 1);
    if (multiSubmesh) {
        GltfAsset rigidView{};
        rigidView.material = asset.material;
        rigidView.materials = asset.materials;
        if (MultiMaterialComponent* multi = owner.AddComponent<MultiMaterialComponent>()) {
            multi->PopulateFromGltfAsset(rigidView);
        }
        return;
    }
    if (MaterialComponent* mat = owner.AddComponent<MaterialComponent>()) {
        if (!asset.materials.IsEmpty()) {
            ApplyGltfMaterialDesc(*mat, asset.materials[0]);
        } else {
            ApplyGltfMaterialDesc(*mat, asset.material);
        }
    }
}

}  // namespace

void GltfAssetBinder::ApplyMaterials(GameObject& owner, const GltfAsset& asset) {
    BindMaterials(owner, asset);
}

void GltfAssetBinder::ApplyMaterials(GameObject& owner, const SkinnedGltfAsset& asset) {
    BindMaterials(owner, asset);
}

void GltfAssetBinder::BindRigidMesh(
        GameObject& owner,
        const GltfAsset& asset,
        const SceneMeshSlot slot,
        const Vector3& albedo) {
    if (!asset.mesh) {
        return;
    }
    if (owner.GetComponent<MeshComponent>() == nullptr) {
        owner.AddComponent<MeshComponent>(asset.mesh, slot, albedo);
    }
    BindMaterials(owner, asset);
}

void GltfAssetBinder::BindSkinnedMesh(
        GameObject& owner,
        const SkinnedGltfAsset& asset,
        const Vector3& /*albedo*/) {
    if (!asset.mesh) {
        return;
    }
    if (owner.GetComponent<SkinnedMeshComponent>() == nullptr) {
        owner.AddComponent<SkinnedMeshComponent>(asset.mesh);
    }
    BindMaterials(owner, asset);
}

}  // namespace Spark
