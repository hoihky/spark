#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"
#include "spark/scene/texture/Texture2D.hpp"

struct cgltf_data;
struct cgltf_material;
struct cgltf_mesh;

namespace Spark {

class MultiMaterialComponent;

/**
 * PBR material data extracted from a glTF 2.0 material (metallic-roughness workflow).
 * Textures are decoded to CPU RGBA8; GPU upload happens at scene submit time.
 */
class GltfMaterial {
public:
    SharedPtr<Texture2D> baseColor;
    SharedPtr<Texture2D> normalMap;
    SharedPtr<Texture2D> metallicRoughness;
    SharedPtr<Texture2D> emissiveMap;

    Vector3 baseColorFactor{Vector3::One};
    float metallicFactor = 1.0F;
    float roughnessFactor = 1.0F;
    Vector3 emissiveFactor{Vector3::One};
    float emissiveIntensity = 0.0F;
    float occlusionStrength = 1.0F;
    bool doubleSided = false;
    float opacity = 1.0F;
    /** glTF alpha_mode=MASK cutoff; 0 = disabled (no alpha test). */
    float alphaCutoff = 0.0F;
    bool unlit = false;
    MaterialUvMap baseColorUv{};
    MaterialUvMap normalUv{};
    MaterialUvMap metallicRoughnessUv{};
    MaterialUvMap emissiveUv{};

    [[nodiscard]] bool HasAnyTexture() const noexcept {
        return static_cast<bool>(baseColor) || static_cast<bool>(normalMap) ||
               static_cast<bool>(metallicRoughness) || static_cast<bool>(emissiveMap);
    }

    /** True when scalar factors or flags differ from implicit glTF defaults (no textures required). */
    [[nodiscard]] bool HasScalarPresentation() const noexcept;

    /** True when this material should be bound for rendering (textures and/or scalar PBR). */
    [[nodiscard]] bool HasPresentationContent() const noexcept {
        return HasAnyTexture() || HasScalarPresentation();
    }

    /** Apply glTF factors and texture slots onto a <c>MaterialComponent</c>. */
    void ApplyTo(MaterialComponent& material) const;

    /** Apply onto one slot of a <c>MultiMaterialComponent</c>. */
    void ApplyTo(MultiMaterialComponent::Slot& slot) const;
};

/** Loads <c>GltfMaterial</c> instances from cgltf parse results. */
class GltfMaterialLoader {
public:
    /**
     * Parse a single glTF material (textures + scalar factors). Returns false when <c>mat</c> is null.
     * Texture debug names are derived from <c>gltfPath</c> and image URIs.
     */
    [[nodiscard]] static bool LoadFromCgltf(
            const cgltf_material* mat,
            const char* gltfPath,
            GltfMaterial& outMaterial);

    /**
     * Load the primary material for a glTF file. When <c>meshHint</c> is set, prefers materials
     * referenced by that mesh's primitives (multi-material glTFs).
     */
    [[nodiscard]] static bool LoadPrimary(
            const cgltf_data* data,
            const cgltf_mesh* meshHint,
            const char* gltfPath,
            GltfMaterial& outMaterial);

    /** Loads every material entry in <c>data->materials</c>. */
    static void LoadAll(const cgltf_data* data, const char* gltfPath, Array<GltfMaterial>& outMaterials);
};

using GltfMaterialDesc = GltfMaterial;

inline void ApplyGltfMaterialDesc(MaterialComponent& material, const GltfMaterial& desc) {
    desc.ApplyTo(material);
}

[[nodiscard]] inline bool TryLoadGltfMaterialFromCgltf(
        const cgltf_material* mat,
        const char* gltfPath,
        GltfMaterial& outMaterial) {
    return GltfMaterialLoader::LoadFromCgltf(mat, gltfPath, outMaterial);
}

[[nodiscard]] inline bool TryLoadPrimaryGltfMaterial(
        const cgltf_data* data,
        const cgltf_mesh* meshHint,
        const char* gltfPath,
        GltfMaterial& outMaterial) {
    return GltfMaterialLoader::LoadPrimary(data, meshHint, gltfPath, outMaterial);
}

}  // namespace Spark
