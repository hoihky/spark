#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/material/MaterialGltfExtensions.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"
#include "spark/scene/material/MaterialLibraryBinding.hpp"

#include <cstdint>

namespace Spark {

class GameWorld;
class Texture2D;

/**
 * Physically inspired material: optional base-color texture, tint, metallic/roughness, optional emissive texture,
 * and HDR emissive (color × intensity × emissive map RGB when a map is set).
 * Values map to the scene shader (GGX specular + Lambert diffuse with metallic energy split).
 */
class MaterialComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Material;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    MaterialComponent() = default;
    MaterialComponent(SharedPtr<Texture2D> baseColor, Vector3 inTint = Vector3::One);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;
    void OnDetach(GameObject& owner) override;

    [[nodiscard]] const SharedPtr<Texture2D>& GetBaseColorTexture() const noexcept { return baseColor; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetNormalTexture() const noexcept { return normalMap; }
    /** glTF-style packed occlusion (R), roughness (G), metallic (B). */
    [[nodiscard]] const SharedPtr<Texture2D>& GetMetallicRoughnessTexture() const noexcept {
        return metallicRoughness;
    }
    [[nodiscard]] const SharedPtr<Texture2D>& GetEmissiveTexture() const noexcept { return emissiveMap; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetIridescenceThicknessTexture() const noexcept {
        return iridescenceThicknessMap;
    }
    [[nodiscard]] const Vector3& GetTint() const noexcept { return tint; }
    [[nodiscard]] float GetMetallic() const noexcept { return metallic; }
    [[nodiscard]] float GetRoughness() const noexcept { return roughness; }
    [[nodiscard]] float GetMetallicFactor() const noexcept { return metallicFactor; }
    [[nodiscard]] float GetRoughnessFactor() const noexcept { return roughnessFactor; }
    [[nodiscard]] float GetOcclusionStrength() const noexcept { return occlusionStrength; }
    [[nodiscard]] const Vector3& GetEmissiveColor() const noexcept { return emissiveColor; }
    [[nodiscard]] float GetEmissiveIntensity() const noexcept { return emissiveIntensity; }
    [[nodiscard]] const Vector3& GetEmissiveFactor() const noexcept { return emissiveFactor; }

    void SetBaseColorTexture(SharedPtr<Texture2D> tex);
    void SetNormalTexture(SharedPtr<Texture2D> tex);
    void SetMetallicRoughnessTexture(SharedPtr<Texture2D> tex);
    void SetEmissiveTexture(SharedPtr<Texture2D> tex);
    void SetIridescenceThicknessTexture(SharedPtr<Texture2D> tex);
    void SetTint(const Vector3& t);
    void SetMetallic(float m);
    void SetRoughness(float r);
    void SetMetallicFactor(float f);
    void SetRoughnessFactor(float f);
    void SetOcclusionStrength(float s);
    void SetEmissive(const Vector3& rgb, float intensity);
    void SetEmissiveFactor(const Vector3& f);

    [[nodiscard]] SceneShadingModel GetShadingModel() const noexcept { return shadingModel; }
    void SetShadingModel(SceneShadingModel s);
    [[nodiscard]] std::int32_t GetToonDiffuseBands() const noexcept { return toonDiffuseBands; }
    void SetToonDiffuseBands(std::int32_t bands);
    [[nodiscard]] float GetToonRimIntensity() const noexcept { return toonRimIntensity; }
    void SetToonRimIntensity(float v);
    [[nodiscard]] float GetToonRimPower() const noexcept { return toonRimPower; }
    void SetToonRimPower(float v);
    [[nodiscard]] bool IsDoubleSided() const noexcept { return doubleSided; }
    void SetDoubleSided(bool v);

    /** 1 = opaque; values below 1 route the draw to <c>SceneRenderParams::transparentDraws</c>. */
    [[nodiscard]] float GetOpacity() const noexcept { return opacity; }
    void SetOpacity(float a);

    /** glTF alpha_mode=MASK cutoff; 0 disables alpha test in the opaque pass. */
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }
    void SetAlphaCutoff(float cutoff);

    /** glTF alpha_mode=BLEND; routes draw to the transparent pass. */
    [[nodiscard]] bool IsAlphaBlend() const noexcept { return alphaBlend; }
    void SetAlphaBlend(bool blend);

    [[nodiscard]] const MaterialUvMap& GetBaseColorUvMap() const noexcept { return baseColorUv; }
    [[nodiscard]] const MaterialUvMap& GetNormalUvMap() const noexcept { return normalUv; }
    [[nodiscard]] const MaterialUvMap& GetMetallicRoughnessUvMap() const noexcept { return metallicRoughnessUv; }
    [[nodiscard]] const MaterialUvMap& GetEmissiveUvMap() const noexcept { return emissiveUv; }
    [[nodiscard]] const MaterialUvMap& GetIridescenceThicknessUvMap() const noexcept {
        return iridescenceThicknessUv;
    }
    void SetBaseColorUvMap(const MaterialUvMap& map) noexcept { baseColorUv = map; }
    void SetNormalUvMap(const MaterialUvMap& map) noexcept { normalUv = map; }
    void SetMetallicRoughnessUvMap(const MaterialUvMap& map) noexcept { metallicRoughnessUv = map; }
    void SetEmissiveUvMap(const MaterialUvMap& map) noexcept { emissiveUv = map; }
    void SetIridescenceThicknessUvMap(const MaterialUvMap& map) noexcept { iridescenceThicknessUv = map; }

    [[nodiscard]] float GetNormalScale() const noexcept { return normalScale; }
    void SetNormalScale(const float scale) noexcept { normalScale = scale; }

    [[nodiscard]] const MaterialGltfExtensions& GetGltfExtensions() const noexcept { return gltfExtensions; }
    void SetGltfExtensions(const MaterialGltfExtensions& ext) noexcept { gltfExtensions = ext; }

    /** Library asset key (e.g. <c>materials/hero.sparkmat</c> or <c>model.glb#material/0</c>). */
    [[nodiscard]] const Utf8String& GetMaterialAssetKey() const noexcept { return libraryBinding.GetKey(); }
    [[nodiscard]] bool HasMaterialAsset() const noexcept { return libraryBinding.HasKey(); }
    void SetMaterialAsset(GameWorld& world, const char* key);
    void ClearMaterialAsset(GameWorld& world);
    /** Applies the cached library asset when async loading has completed. */
    void TryApplyMaterialAsset(GameWorld& world);

private:
    void NotifyMaterialChanged();

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
    MaterialLibraryBinding libraryBinding;
};

}  // namespace Spark
