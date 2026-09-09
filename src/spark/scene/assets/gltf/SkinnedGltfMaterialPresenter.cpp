#include "spark/scene/assets/gltf/SkinnedGltfMaterialPresenter.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/assets/GltfAssetPathResolver.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"

namespace Spark {

class SkinnedGltfMaterialPresenter::SingleMaterialBinding {
public:
    void Apply(
            GameObject& owner,
            const GltfMaterial& material,
            const std::uint32_t materialIndex,
            const Utf8String& libraryKey) const {
        if (!material.HasPresentationContent()) {
            return;
        }
        MaterialComponent* component = owner.GetComponent<MaterialComponent>();
        if (component == nullptr) {
            component = owner.AddComponent<MaterialComponent>();
        }
        material.ApplyTo(*component);
        if (!libraryKey.IsEmpty()) {
            const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(libraryKey.CStr(), materialIndex);
            component->SetMaterialAsset(owner.GetWorld(), key.CStr());
        }
    }
};

class SkinnedGltfMaterialPresenter::MultiMaterialBinding {
public:
    void Apply(GameObject& owner, const SkinnedGltfAsset& asset, const Utf8String& libraryKey) const {
        MultiMaterialComponent* multi = owner.GetComponent<MultiMaterialComponent>();
        if (multi == nullptr) {
            multi = owner.AddComponent<MultiMaterialComponent>();
        }
        multi->BindFromSkinnedGltfAsset(owner.GetWorld(), libraryKey.CStr(), asset);
    }
};

bool SkinnedGltfMaterialPresenter::ShouldUseMultiMaterial(const SkinnedGltfAsset& asset) const noexcept {
    return asset.mesh && asset.mesh->GetSubmeshes().GetSize() > 1U;
}

std::uint32_t SkinnedGltfMaterialPresenter::ResolvePrimaryMaterialIndex(const SkinnedGltfAsset& asset) const noexcept {
    if (asset.mesh && !asset.mesh->GetSubmeshes().IsEmpty()) {
        return asset.mesh->GetSubmeshes()[0].materialIndex;
    }
    return 0U;
}

const GltfMaterial& SkinnedGltfMaterialPresenter::ResolveMaterial(
        const SkinnedGltfAsset& asset,
        const std::uint32_t materialIndex) const noexcept {
    if (!asset.materials.IsEmpty()) {
        const std::size_t resolvedIndex =
                materialIndex < asset.materials.GetSize() ? materialIndex : 0U;
        return asset.materials[resolvedIndex];
    }
    return asset.material;
}

Utf8String SkinnedGltfMaterialPresenter::ResolveLibraryKey(const char* gltfLibraryKey) const noexcept {
    if (gltfLibraryKey == nullptr || gltfLibraryKey[0] == '\0') {
        return {};
    }
    Utf8String relative{};
    if (GltfAssetPathResolver::TryMakeAssetsRelative(gltfLibraryKey, relative)) {
        return relative;
    }
    return Utf8String(gltfLibraryKey);
}

void SkinnedGltfMaterialPresenter::PresentOn(
        GameObject& owner,
        const SkinnedGltfAsset& asset,
        const char* gltfLibraryKey) {
    if (!asset.mesh) {
        return;
    }
    const Utf8String libraryKey = ResolveLibraryKey(gltfLibraryKey);
    if (ShouldUseMultiMaterial(asset)) {
        MultiMaterialBinding{}.Apply(owner, asset, libraryKey);
        return;
    }
    const std::uint32_t materialIndex = ResolvePrimaryMaterialIndex(asset);
    const GltfMaterial& primaryMaterial = ResolveMaterial(asset, materialIndex);
    SingleMaterialBinding{}.Apply(owner, primaryMaterial, materialIndex, libraryKey);
}

}  // namespace Spark
