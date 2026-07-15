#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/SceneShadingModel.hpp"

#include <cstdint>

namespace Spark {

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

    [[nodiscard]] const SharedPtr<Texture2D>& GetBaseColorTexture() const noexcept { return baseColor; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetNormalTexture() const noexcept { return normalMap; }
    /** glTF-style packed occlusion (R), roughness (G), metallic (B). */
    [[nodiscard]] const SharedPtr<Texture2D>& GetMetallicRoughnessTexture() const noexcept {
        return metallicRoughness;
    }
    [[nodiscard]] const SharedPtr<Texture2D>& GetEmissiveTexture() const noexcept { return emissiveMap; }
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

private:
    void NotifyMaterialChanged();

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
    std::int32_t toonDiffuseBands = 3;
    float toonRimIntensity = 0.35F;
    float toonRimPower = 4.0F;
    bool doubleSided = false;
    float opacity = 1.0F;
};

}  // namespace Spark
