#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/material/MaterialGltfExtensions.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"
#include "spark/scene/material/MaterialLibraryBinding.hpp"
#include "spark/core/Optional.hpp"
#include "spark/render/lighting/SceneShadowParticipation.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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
    SPARK_SCRIPT_BIND(get_tint)
    [[nodiscard]] const Vector3& GetTint() const noexcept { return tint; }
    SPARK_SCRIPT_BIND(get_metallic)
    [[nodiscard]] float GetMetallic() const noexcept { return metallic; }
    SPARK_SCRIPT_BIND(get_roughness)
    [[nodiscard]] float GetRoughness() const noexcept { return roughness; }
    SPARK_SCRIPT_BIND(get_metallic_factor)
    [[nodiscard]] float GetMetallicFactor() const noexcept { return metallicFactor; }
    SPARK_SCRIPT_BIND(get_roughness_factor)
    [[nodiscard]] float GetRoughnessFactor() const noexcept { return roughnessFactor; }
    SPARK_SCRIPT_BIND(get_occlusion_strength)
    [[nodiscard]] float GetOcclusionStrength() const noexcept { return occlusionStrength; }
    SPARK_SCRIPT_BIND(get_emissive_color)
    [[nodiscard]] const Vector3& GetEmissiveColor() const noexcept { return emissiveColor; }
    SPARK_SCRIPT_BIND(get_emissive_intensity)
    [[nodiscard]] float GetEmissiveIntensity() const noexcept { return emissiveIntensity; }
    SPARK_SCRIPT_BIND(get_emissive_factor)
    [[nodiscard]] const Vector3& GetEmissiveFactor() const noexcept { return emissiveFactor; }

    void SetBaseColorTexture(SharedPtr<Texture2D> tex);
    void SetNormalTexture(SharedPtr<Texture2D> tex);
    void SetMetallicRoughnessTexture(SharedPtr<Texture2D> tex);
    void SetEmissiveTexture(SharedPtr<Texture2D> tex);
    void SetIridescenceThicknessTexture(SharedPtr<Texture2D> tex);
    void SetTint(const Vector3& t);
    void SetMetallic(float m);
    void SetRoughness(float r);
    SPARK_SCRIPT_BIND(set_metallic_factor)
    void SetMetallicFactor(float f);
    SPARK_SCRIPT_BIND(set_roughness_factor)
    void SetRoughnessFactor(float f);
    SPARK_SCRIPT_BIND(set_occlusion_strength)
    void SetOcclusionStrength(float s);
    void SetEmissive(const Vector3& rgb, float intensity);
    SPARK_SCRIPT_BIND(set_emissive_factor)
    void SetEmissiveFactor(const Vector3& f);

    SPARK_SCRIPT_BIND(get_shading_model)
    [[nodiscard]] SceneShadingModel GetShadingModel() const noexcept { return shadingModel; }
    SPARK_SCRIPT_BIND(set_shading_model)
    void SetShadingModel(SceneShadingModel s);
    SPARK_SCRIPT_BIND(get_toon_diffuse_bands)
    [[nodiscard]] std::int32_t GetToonDiffuseBands() const noexcept { return toonDiffuseBands; }
    SPARK_SCRIPT_BIND(set_toon_diffuse_bands)
    void SetToonDiffuseBands(std::int32_t bands);
    SPARK_SCRIPT_BIND(get_toon_rim_intensity)
    [[nodiscard]] float GetToonRimIntensity() const noexcept { return toonRimIntensity; }
    SPARK_SCRIPT_BIND(set_toon_rim_intensity)
    void SetToonRimIntensity(float v);
    SPARK_SCRIPT_BIND(get_toon_rim_power)
    [[nodiscard]] float GetToonRimPower() const noexcept { return toonRimPower; }
    SPARK_SCRIPT_BIND(set_toon_rim_power)
    void SetToonRimPower(float v);
    SPARK_SCRIPT_BIND(is_double_sided)
    [[nodiscard]] bool IsDoubleSided() const noexcept { return doubleSided; }
    SPARK_SCRIPT_BIND(set_double_sided)
    void SetDoubleSided(bool v);

    /** 1 = opaque; values below 1 route the draw to <c>SceneRenderParams::transparentDraws</c>. */
    SPARK_SCRIPT_BIND(get_opacity)
    [[nodiscard]] float GetOpacity() const noexcept { return opacity; }
    SPARK_SCRIPT_BIND(set_opacity)
    void SetOpacity(float a);

    /** glTF alpha_mode=MASK cutoff; 0 disables alpha test in the opaque pass. */
    SPARK_SCRIPT_BIND(get_alpha_cutoff)
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }
    SPARK_SCRIPT_BIND(set_alpha_cutoff)
    void SetAlphaCutoff(float cutoff);

    /** glTF alpha_mode=BLEND; routes draw to the transparent pass. */
    SPARK_SCRIPT_BIND(is_alpha_blend)
    [[nodiscard]] bool IsAlphaBlend() const noexcept { return alphaBlend; }
    SPARK_SCRIPT_BIND(set_alpha_blend)
    void SetAlphaBlend(bool blend);

    SPARK_SCRIPT_BIND(get_base_color_uv_map)
    [[nodiscard]] const MaterialUvMap& GetBaseColorUvMap() const noexcept { return baseColorUv; }
    SPARK_SCRIPT_BIND(get_normal_uv_map)
    [[nodiscard]] const MaterialUvMap& GetNormalUvMap() const noexcept { return normalUv; }
    SPARK_SCRIPT_BIND(get_metallic_roughness_uv_map)
    [[nodiscard]] const MaterialUvMap& GetMetallicRoughnessUvMap() const noexcept { return metallicRoughnessUv; }
    SPARK_SCRIPT_BIND(get_emissive_uv_map)
    [[nodiscard]] const MaterialUvMap& GetEmissiveUvMap() const noexcept { return emissiveUv; }
    [[nodiscard]] const MaterialUvMap& GetIridescenceThicknessUvMap() const noexcept {
        return iridescenceThicknessUv;
    }
    SPARK_SCRIPT_BIND(set_base_color_uv_map)
    void SetBaseColorUvMap(const MaterialUvMap& map) noexcept { baseColorUv = map; }
    SPARK_SCRIPT_BIND(set_normal_uv_map)
    void SetNormalUvMap(const MaterialUvMap& map) noexcept { normalUv = map; }
    SPARK_SCRIPT_BIND(set_metallic_roughness_uv_map)
    void SetMetallicRoughnessUvMap(const MaterialUvMap& map) noexcept { metallicRoughnessUv = map; }
    SPARK_SCRIPT_BIND(set_emissive_uv_map)
    void SetEmissiveUvMap(const MaterialUvMap& map) noexcept { emissiveUv = map; }
    SPARK_SCRIPT_BIND(set_iridescence_thickness_uv_map)
    void SetIridescenceThicknessUvMap(const MaterialUvMap& map) noexcept { iridescenceThicknessUv = map; }

    SPARK_SCRIPT_BIND(get_normal_scale)
    [[nodiscard]] float GetNormalScale() const noexcept { return normalScale; }
    SPARK_SCRIPT_BIND(set_normal_scale)
    void SetNormalScale(const float scale) noexcept { normalScale = scale; }

    SPARK_SCRIPT_BIND(get_gltf_extensions)
    [[nodiscard]] const MaterialGltfExtensions& GetGltfExtensions() const noexcept { return gltfExtensions; }
    SPARK_SCRIPT_BIND(set_gltf_extensions)
    void SetGltfExtensions(const MaterialGltfExtensions& ext) noexcept { gltfExtensions = ext; }

    /** Library asset key (e.g. <c>materials/hero.sparkmat</c> or <c>model.glb#material/0</c>). */
    SPARK_SCRIPT_BIND(get_material_asset_key)
    [[nodiscard]] const Utf8String& GetMaterialAssetKey() const noexcept { return libraryBinding.GetKey(); }
    SPARK_SCRIPT_BIND(has_s_material_asset)
    [[nodiscard]] bool HasMaterialAsset() const noexcept { return libraryBinding.HasKey(); }
    void SetMaterialAsset(GameWorld& world, const char* key);
    void ClearMaterialAsset(GameWorld& world);
    /** Applies the cached library asset when async loading has completed. */
    void TryApplyMaterialAsset(GameWorld& world);

    /**
     * Optional per-material shadow participation overrides (directional CSM).
     * When unset, scene submit uses `SceneRenderParams::shadowsCastByDefault` /
     * `shadowsReceiveByDefault` plus built-in rules (terrain always casts; submerged
     * ground planes and underwater unit cubes do not cast).
     */
    SPARK_SCRIPT_BIND(set_shadow_cast_override)
    void SetShadowCastOverride(const bool casts) noexcept { shadowCastOverride = casts; }
    SPARK_SCRIPT_BIND(clear_shadow_cast_override)
    void ClearShadowCastOverride() noexcept { shadowCastOverride.Reset(); }
    SPARK_SCRIPT_BIND(get_shadow_cast_override)
    [[nodiscard]] Optional<bool> GetShadowCastOverride() const noexcept { return shadowCastOverride; }

    SPARK_SCRIPT_BIND(set_shadow_receive_override)
    void SetShadowReceiveOverride(const bool receives) noexcept { shadowReceiveOverride = receives; }
    SPARK_SCRIPT_BIND(clear_shadow_receive_override)
    void ClearShadowReceiveOverride() noexcept { shadowReceiveOverride.Reset(); }
    [[nodiscard]] Optional<bool> GetShadowReceiveOverride() const noexcept {
        return shadowReceiveOverride;
    }

    /** Unified shadow API; wraps cast/receive overrides above. */
    void SetShadowParticipation(const SceneShadowParticipation& participation) noexcept {
        participation.ApplyTo(*this);
    }
    [[nodiscard]] SceneShadowParticipation GetShadowParticipation() const noexcept {
        return SceneShadowParticipation::FromMaterial(*this);
    }

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
    Optional<bool> shadowCastOverride{};
    Optional<bool> shadowReceiveOverride{};
};

}  // namespace Spark
