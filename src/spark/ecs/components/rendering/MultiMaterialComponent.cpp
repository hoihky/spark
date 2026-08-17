#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"

#include "spark/scene/GameWorldAssetCache.hpp"
#include "spark/scene/GltfMaterial.hpp"

namespace Spark {

void MultiMaterialComponent::ResizeSlots(const std::size_t count) {
    slots.Resize(count);
}

void MultiMaterialComponent::PopulateFromGltfAsset(const GltfAsset& asset) {
    slots.Clear();
    if (asset.materials.IsEmpty()) {
        if (asset.material.HasAnyTexture()) {
            slots.Resize(1);
            asset.material.ApplyTo(slots[0]);
        }
        return;
    }
    slots.Resize(asset.materials.GetSize());
    for (std::size_t i = 0; i < asset.materials.GetSize(); ++i) {
        asset.materials[i].ApplyTo(slots[i]);
    }
}

}  // namespace Spark
