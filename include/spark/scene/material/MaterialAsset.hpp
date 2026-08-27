#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/material/MaterialGltfExtensions.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

class MaterialComponent;
class MultiMaterialComponent;
class GltfMaterial;

/**
 * Shared PBR material definition (textures + factors). Stored in <c>GameWorldAssetCache</c>
 * and referenced by <c>MaterialComponent::materialAssetKey</c> for instance reuse.
 */
class MaterialAsset {
public:
    Utf8String name;

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
    MaterialGltfExtensions gltfExtensions{};
    MaterialUvMap baseColorUv{};
    MaterialUvMap normalUv{};
    MaterialUvMap metallicRoughnessUv{};
    MaterialUvMap emissiveUv{};
    MaterialUvMap iridescenceThicknessUv{};
    float normalScale = 1.0F;

    [[nodiscard]] bool HasAnyTexture() const noexcept;

    void CopyFromGltfMaterial(const GltfMaterial& source);
    void CaptureFromMaterial(const MaterialComponent& material);
    void ApplyTo(MaterialComponent& material) const;
    void ApplyTo(MultiMaterialComponent::Slot& slot) const;

    [[nodiscard]] bool ApproximatelyEquals(const MaterialComponent& material) const noexcept;
};

}  // namespace Spark
