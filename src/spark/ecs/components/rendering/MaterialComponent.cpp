#include "spark/ecs/components/rendering/MaterialComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/MaterialAsset.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

MaterialComponent::MaterialComponent(SharedPtr<Texture2D> inBaseColor, Vector3 inTint)
    : baseColor(MoveTemp(inBaseColor)), tint(inTint) {}

void MaterialComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {
}

void MaterialComponent::OnDetach(GameObject& owner) {
    ClearMaterialAsset(owner.GetWorld());
}

void MaterialComponent::NotifyMaterialChanged() {
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

void MaterialComponent::SetBaseColorTexture(SharedPtr<Texture2D> tex) {
    baseColor = MoveTemp(tex);
    NotifyMaterialChanged();
}

void MaterialComponent::SetNormalTexture(SharedPtr<Texture2D> tex) {
    normalMap = MoveTemp(tex);
    NotifyMaterialChanged();
}

void MaterialComponent::SetMetallicRoughnessTexture(SharedPtr<Texture2D> tex) {
    metallicRoughness = MoveTemp(tex);
    NotifyMaterialChanged();
}

void MaterialComponent::SetEmissiveTexture(SharedPtr<Texture2D> tex) {
    emissiveMap = MoveTemp(tex);
    NotifyMaterialChanged();
}

void MaterialComponent::SetTint(const Vector3& t) {
    tint = t;
    NotifyMaterialChanged();
}

void MaterialComponent::SetMetallic(float m) {
    metallic = m;
    NotifyMaterialChanged();
}

void MaterialComponent::SetRoughness(float r) {
    roughness = r;
    NotifyMaterialChanged();
}

void MaterialComponent::SetMetallicFactor(const float f) {
    metallicFactor = f;
    NotifyMaterialChanged();
}

void MaterialComponent::SetRoughnessFactor(const float f) {
    roughnessFactor = f;
    NotifyMaterialChanged();
}

void MaterialComponent::SetOcclusionStrength(const float s) {
    occlusionStrength = s;
    NotifyMaterialChanged();
}

void MaterialComponent::SetEmissive(const Vector3& rgb, const float intensity) {
    emissiveColor = rgb;
    emissiveIntensity = intensity;
    NotifyMaterialChanged();
}

void MaterialComponent::SetEmissiveFactor(const Vector3& f) {
    emissiveFactor = f;
    NotifyMaterialChanged();
}

void MaterialComponent::SetShadingModel(const SceneShadingModel s) {
    shadingModel = s;
    NotifyMaterialChanged();
}

void MaterialComponent::SetToonDiffuseBands(const std::int32_t bands) {
    toonDiffuseBands = bands;
    NotifyMaterialChanged();
}

void MaterialComponent::SetToonRimIntensity(const float v) {
    toonRimIntensity = v;
    NotifyMaterialChanged();
}

void MaterialComponent::SetToonRimPower(const float v) {
    toonRimPower = v;
    NotifyMaterialChanged();
}

void MaterialComponent::SetDoubleSided(const bool v) {
    doubleSided = v;
    NotifyMaterialChanged();
}

void MaterialComponent::SetOpacity(const float a) {
    opacity = a < 0.0F ? 0.0F : (a > 1.0F ? 1.0F : a);
    NotifyMaterialChanged();
}

void MaterialComponent::SetAlphaCutoff(const float cutoff) {
    alphaCutoff = cutoff < 0.0F ? 0.0F : (cutoff > 1.0F ? 1.0F : cutoff);
    NotifyMaterialChanged();
}

void MaterialComponent::SetMaterialAsset(GameWorld& world, const char* key) {
    ClearMaterialAsset(world);
    materialAssetKey = (key != nullptr && key[0] != '\0') ? Utf8String(key) : Utf8String{};
    if (materialAssetKey.IsEmpty()) {
        pendingMaterialApply = false;
        return;
    }
    world.RetainAsset(CachedAssetKind::Material, materialAssetKey.CStr());
    pendingMaterialApply = true;
    if (world.TryGetMaterialByKeyOrPath(materialAssetKey.CStr()) == nullptr) {
        world.RequestMaterial(materialAssetKey.CStr());
    }
    TryApplyMaterialAsset(world);
}

void MaterialComponent::ClearMaterialAsset(GameWorld& world) {
    if (!materialAssetKey.IsEmpty()) {
        world.ReleaseAsset(CachedAssetKind::Material, materialAssetKey.CStr());
        materialAssetKey = {};
    }
    pendingMaterialApply = false;
}

void MaterialComponent::TryApplyMaterialAsset(GameWorld& world) {
    if (!pendingMaterialApply || materialAssetKey.IsEmpty()) {
        return;
    }
    if (const MaterialAsset* asset = world.TryGetMaterialByKeyOrPath(materialAssetKey.CStr())) {
        asset->ApplyTo(*this);
        pendingMaterialApply = false;
    }
}

}  // namespace Spark
