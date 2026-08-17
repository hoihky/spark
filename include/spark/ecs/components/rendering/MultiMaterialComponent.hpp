#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"

namespace Spark {

class Texture2D;
struct GltfAsset;

/**
 * Per-submesh material slots for multi-material glTF meshes.
 * Indexed by glTF material index (<c>MeshSubmesh::materialIndex</c>).
 */
class MultiMaterialComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Unknown;

    struct Slot {
        SharedPtr<Texture2D> baseColor;
        SharedPtr<Texture2D> normalMap;
        SharedPtr<Texture2D> metallicRoughness;
        SharedPtr<Texture2D> emissiveMap;
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
        bool doubleSided = false;
        float opacity = 1.0F;
    };

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void Clear() noexcept { slots.Clear(); }
    void ResizeSlots(std::size_t count);
    [[nodiscard]] std::size_t GetSlotCount() const noexcept { return slots.GetSize(); }
    [[nodiscard]] Slot& GetSlot(std::size_t index) { return slots[index]; }
    [[nodiscard]] const Slot& GetSlot(std::size_t index) const { return slots[index]; }

    /** Sizes slots to the glTF material table and copies textures/factors from the asset. */
    void PopulateFromGltfAsset(const GltfAsset& asset);

private:
    Array<Slot> slots;
};

}  // namespace Spark
