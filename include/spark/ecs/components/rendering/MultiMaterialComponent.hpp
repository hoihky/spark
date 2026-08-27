#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/material/MaterialGltfExtensions.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"
#include "spark/scene/material/MaterialLibraryBinding.hpp"

namespace Spark {

class Texture2D;
class GameWorld;
struct GltfAsset;

/**
 * Per-submesh material slots for multi-material glTF meshes.
 * Indexed by glTF material index (<c>MeshSubmesh::materialIndex</c>).
 */
class MultiMaterialComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::MultiMaterial;

    struct Slot {
        SharedPtr<Texture2D> baseColor;
        SharedPtr<Texture2D> normalMap;
        SharedPtr<Texture2D> metallicRoughness;
        SharedPtr<Texture2D> emissiveMap;
        SharedPtr<Texture2D> iridescenceThicknessMap;
        Vector3 tint{Vector3::One};
        float metallic = 0.0F;
        float roughness = 0.45F;
        float metallicFactor = 1.0F;
        float roughnessFactor = 1.0F;
        float occlusionStrength = 1.0F;
        Vector3 emissiveColor{};
        float emissiveIntensity = 0.0F;
        Vector3 emissiveFactor{Vector3::One};
        SceneShadingModel shadingModel = SceneShadingModel::LitPbr;
        std::int32_t toonDiffuseBands = 3;
        float toonRimIntensity = 0.35F;
        float toonRimPower = 4.0F;
        bool doubleSided = false;
        float opacity = 1.0F;
        float alphaCutoff = 0.0F;
        bool alphaBlend = false;
        MaterialUvMap baseColorUv{};
        MaterialUvMap normalUv{};
        MaterialUvMap metallicRoughnessUv{};
        MaterialUvMap emissiveUv{};
        MaterialUvMap iridescenceThicknessUv{};
        float normalScale = 1.0F;
        MaterialGltfExtensions gltfExtensions{};
        Utf8String materialAssetKey;
    };

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnDetach(GameObject& owner) override;

    void Clear() noexcept;
    void ResizeSlots(std::size_t count);
    [[nodiscard]] std::size_t GetSlotCount() const noexcept { return slots.GetSize(); }
    [[nodiscard]] Slot& GetSlot(std::size_t index) { return slots[index]; }
    [[nodiscard]] const Slot& GetSlot(std::size_t index) const { return slots[index]; }

    [[nodiscard]] const Utf8String& GetSlotMaterialAssetKey(std::size_t index) const;
    [[nodiscard]] bool SlotHasMaterialAsset(std::size_t index) const noexcept;

    void SetSlotMaterialAsset(GameWorld& world, std::size_t index, const char* key);
    void ClearSlotMaterialAsset(GameWorld& world, std::size_t index);
    void ClearAllMaterialAssets(GameWorld& world);
    void TryApplyMaterialAssets(GameWorld& world);

    [[nodiscard]] std::uint32_t GetActiveVariantIndex() const noexcept { return activeVariantIndex; }
    [[nodiscard]] const Array<Utf8String>& GetVariantNames() const noexcept { return variantNames; }
    [[nodiscard]] bool HasMaterialVariants() const noexcept { return variantNames.GetSize() > 1U; }
    void SetVariantNames(const Array<Utf8String>& names);
    void SetActiveVariantIndex(const std::uint32_t index);
    void CycleActiveVariant();

    /** Advances variant index once and applies it to every variant-capable slot on <c>root</c> and descendants. */
    static void CycleVariantsOnObjectTree(GameObject* root);

    /** Sizes slots to the glTF material table and copies textures/factors from the asset. */
    void PopulateFromGltfAsset(const GltfAsset& asset);

    /**
     * Sizes slots and binds <c>gltfPath#material/N</c> library keys when <c>gltfPath</c> is set;
     * otherwise falls back to <c>PopulateFromGltfAsset</c>.
     */
    void BindFromGltfAsset(GameWorld& world, const char* gltfPath, const GltfAsset& asset);

private:
    void NotifyMaterialChanged();
    void EnsureSlotAuxSize(std::size_t count);
    void ReleaseSlotBinding(GameWorld& world, std::size_t index);

    Array<Slot> slots;
    Array<MaterialLibraryBinding> slotBindings;
    Array<Utf8String> variantNames;
    std::uint32_t activeVariantIndex = 0;
};

}  // namespace Spark
