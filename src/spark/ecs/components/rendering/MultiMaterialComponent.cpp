#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"
#include "spark/scene/material/MaterialAsset.hpp"

namespace Spark {

void MultiMaterialComponent::OnDetach(GameObject& owner) {
    ClearAllMaterialAssets(owner.GetWorld());
}

void MultiMaterialComponent::Clear() noexcept {
    slots.Clear();
    slotBindings.Clear();
    variantNames.Clear();
    activeVariantIndex = 0;
}

void MultiMaterialComponent::ResizeSlots(const std::size_t count) {
    slots.Resize(count);
    EnsureSlotAuxSize(count);
}

void MultiMaterialComponent::EnsureSlotAuxSize(const std::size_t count) {
    if (slotBindings.GetSize() < count) {
        slotBindings.Resize(count);
    }
}

void MultiMaterialComponent::NotifyMaterialChanged() {
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        owner->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

void MultiMaterialComponent::ReleaseSlotBinding(GameWorld& world, const std::size_t index) {
    if (index < slotBindings.GetSize()) {
        slotBindings[index].Release(world);
    }
    if (index < slots.GetSize()) {
        slots[index].materialAssetKey = {};
    }
}

const Utf8String& MultiMaterialComponent::GetSlotMaterialAssetKey(const std::size_t index) const {
    static const Utf8String kEmpty{};
    if (index >= slots.GetSize()) {
        return kEmpty;
    }
    return slots[index].materialAssetKey;
}

bool MultiMaterialComponent::SlotHasMaterialAsset(const std::size_t index) const noexcept {
    return index < slots.GetSize() && !slots[index].materialAssetKey.IsEmpty();
}

void MultiMaterialComponent::SetSlotMaterialAsset(
        GameWorld& world,
        const std::size_t index,
        const char* key) {
    if (index >= slots.GetSize()) {
        return;
    }
    ReleaseSlotBinding(world, index);
    EnsureSlotAuxSize(index + 1U);
    MaterialLibraryBinding& binding = slotBindings[index];
    binding.Assign(world, key);
    slots[index].materialAssetKey = binding.GetKey();
    binding.TryApply(world, [&](const MaterialAsset& asset) { asset.ApplyTo(slots[index]); });
    NotifyMaterialChanged();
}

void MultiMaterialComponent::ClearSlotMaterialAsset(GameWorld& world, const std::size_t index) {
    if (index >= slots.GetSize()) {
        return;
    }
    ReleaseSlotBinding(world, index);
    NotifyMaterialChanged();
}

void MultiMaterialComponent::ClearAllMaterialAssets(GameWorld& world) {
    for (std::size_t i = 0; i < slots.GetSize(); ++i) {
        ReleaseSlotBinding(world, i);
    }
}

void MultiMaterialComponent::TryApplyMaterialAssets(GameWorld& world) {
    for (std::size_t i = 0; i < slots.GetSize(); ++i) {
        if (i >= slotBindings.GetSize()) {
            continue;
        }
        slotBindings[i].TryApply(world, [&](const MaterialAsset& asset) { asset.ApplyTo(slots[i]); });
    }
}

void MultiMaterialComponent::PopulateFromGltfAsset(const GltfAsset& asset) {
    slots.Clear();
    slotBindings.Clear();
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

void MultiMaterialComponent::PopulateFromSkinnedGltfAsset(const SkinnedGltfAsset& asset) {
    slots.Clear();
    slotBindings.Clear();
    if (asset.materials.IsEmpty()) {
        if (asset.material.HasPresentationContent()) {
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

void MultiMaterialComponent::BindFromSkinnedGltfAsset(
        GameWorld& world,
        const char* gltfPath,
        const SkinnedGltfAsset& asset) {
    Clear();
    if (asset.materials.IsEmpty()) {
        if (asset.material.HasPresentationContent()) {
            slots.Resize(1);
            asset.material.ApplyTo(slots[0]);
        }
        return;
    }

    PopulateFromSkinnedGltfAsset(asset);
    SetVariantNames(asset.materialVariantNames);

    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return;
    }

    EnsureSlotAuxSize(slots.GetSize());
    for (std::size_t i = 0; i < slots.GetSize(); ++i) {
        const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(gltfPath, i);
        slotBindings[i].Retain(world, key.CStr());
        slots[i].materialAssetKey = key;
    }
}

void MultiMaterialComponent::BindFromGltfAsset(
        GameWorld& world,
        const char* gltfPath,
        const GltfAsset& asset) {
    Clear();
    if (asset.materials.IsEmpty()) {
        if (asset.material.HasPresentationContent()) {
            slots.Resize(1);
            asset.material.ApplyTo(slots[0]);
        }
        return;
    }

    PopulateFromGltfAsset(asset);
    SetVariantNames(asset.materialVariantNames);

    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        return;
    }

    EnsureSlotAuxSize(slots.GetSize());
    for (std::size_t i = 0; i < slots.GetSize(); ++i) {
        const Utf8String key = MaterialAssetLoader::MakeGltfMaterialLibraryKey(gltfPath, i);
        slotBindings[i].Retain(world, key.CStr());
        slots[i].materialAssetKey = key;
    }
}

void MultiMaterialComponent::SetVariantNames(const Array<Utf8String>& names) {
    variantNames = names;
    if (activeVariantIndex >= variantNames.GetSize()) {
        activeVariantIndex = 0;
    }
    NotifyMaterialChanged();
}

void MultiMaterialComponent::SetActiveVariantIndex(const std::uint32_t index) {
    if (variantNames.IsEmpty()) {
        activeVariantIndex = 0;
        return;
    }
    activeVariantIndex = index % static_cast<std::uint32_t>(variantNames.GetSize());
    NotifyMaterialChanged();
}

void MultiMaterialComponent::CycleActiveVariant() {
    if (variantNames.GetSize() <= 1U) {
        return;
    }
    activeVariantIndex = (activeVariantIndex + 1U) % static_cast<std::uint32_t>(variantNames.GetSize());
    NotifyMaterialChanged();
}

void MultiMaterialComponent::CycleVariantsOnObjectTree(GameObject* root) {
    if (root == nullptr) {
        return;
    }
    Array<MultiMaterialComponent*> multis;
    Array<GameObject*> stack;
    stack.PushBack(root);
    while (!stack.IsEmpty()) {
        GameObject* obj = stack.GetLast();
        stack.PopBack();
        if (obj == nullptr) {
            continue;
        }
        if (MultiMaterialComponent* multi = obj->GetComponent<MultiMaterialComponent>()) {
            if (multi->HasMaterialVariants()) {
                multis.PushBack(multi);
            }
        }
        const Array<GameObject*>& children = obj->GetChildren();
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            stack.PushBack(children[i]);
        }
    }
    if (multis.IsEmpty()) {
        return;
    }
    multis[0]->CycleActiveVariant();
    const std::uint32_t index = multis[0]->GetActiveVariantIndex();
    for (std::size_t i = 1; i < multis.GetSize(); ++i) {
        multis[i]->SetActiveVariantIndex(index);
    }
}

}  // namespace Spark
