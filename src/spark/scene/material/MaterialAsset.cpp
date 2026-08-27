#include "spark/scene/material/MaterialAsset.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include <cmath>

namespace Spark {

namespace {

bool ApproximatelyEqual(const float a, const float b) noexcept {
    return std::fabs(a - b) <= 1.0e-5F;
}

bool ApproximatelyEqual(const Vector3& a, const Vector3& b) noexcept {
    return ApproximatelyEqual(a.x, b.x) && ApproximatelyEqual(a.y, b.y) && ApproximatelyEqual(a.z, b.z);
}

bool SharedPtrEqual(const SharedPtr<Texture2D>& a, const SharedPtr<Texture2D>& b) noexcept {
    return a.Get() == b.Get();
}

}  // namespace

bool MaterialAsset::HasAnyTexture() const noexcept {
    return static_cast<bool>(baseColor) || static_cast<bool>(normalMap) ||
           static_cast<bool>(metallicRoughness) || static_cast<bool>(emissiveMap);
}

void MaterialAsset::CopyFromGltfMaterial(const GltfMaterial& source) {
    baseColor = source.baseColor;
    normalMap = source.normalMap;
    metallicRoughness = source.metallicRoughness;
    emissiveMap = source.emissiveMap;
    iridescenceThicknessMap = source.iridescenceThicknessMap;
    tint = source.baseColorFactor;
    metallicFactor = source.metallicFactor;
    roughnessFactor = source.roughnessFactor;
    if (source.metallicRoughness) {
        metallic = 1.0F;
        roughness = 1.0F;
    } else {
        metallic = source.metallicFactor;
        roughness = source.roughnessFactor;
        metallicFactor = 1.0F;
        roughnessFactor = 1.0F;
    }
    occlusionStrength = source.occlusionStrength;
    emissiveColor = source.emissiveFactor;
    emissiveIntensity = source.emissiveIntensity;
    emissiveFactor = source.emissiveFactor;
    doubleSided = source.doubleSided;
    opacity = source.opacity;
    alphaCutoff = source.alphaCutoff;
    alphaBlend = source.alphaBlend;
    gltfExtensions = source.gltfExtensions;
    baseColorUv = source.baseColorUv;
    normalUv = source.normalUv;
    metallicRoughnessUv = source.metallicRoughnessUv;
    emissiveUv = source.emissiveUv;
    iridescenceThicknessUv = source.iridescenceThicknessUv;
    normalScale = source.normalScale;
    if (source.unlit) {
        shadingModel = SceneShadingModel::Unlit;
    }
}

void MaterialAsset::CaptureFromMaterial(const MaterialComponent& material) {
    baseColor = material.GetBaseColorTexture();
    normalMap = material.GetNormalTexture();
    metallicRoughness = material.GetMetallicRoughnessTexture();
    emissiveMap = material.GetEmissiveTexture();
    iridescenceThicknessMap = material.GetIridescenceThicknessTexture();
    tint = material.GetTint();
    metallic = material.GetMetallic();
    roughness = material.GetRoughness();
    metallicFactor = material.GetMetallicFactor();
    roughnessFactor = material.GetRoughnessFactor();
    occlusionStrength = material.GetOcclusionStrength();
    emissiveColor = material.GetEmissiveColor();
    emissiveIntensity = material.GetEmissiveIntensity();
    emissiveFactor = material.GetEmissiveFactor();
    shadingModel = material.GetShadingModel();
    toonDiffuseBands = material.GetToonDiffuseBands();
    toonRimIntensity = material.GetToonRimIntensity();
    toonRimPower = material.GetToonRimPower();
    doubleSided = material.IsDoubleSided();
    opacity = material.GetOpacity();
    alphaCutoff = material.GetAlphaCutoff();
    alphaBlend = material.IsAlphaBlend();
    gltfExtensions = material.GetGltfExtensions();
    baseColorUv = material.GetBaseColorUvMap();
    normalUv = material.GetNormalUvMap();
    metallicRoughnessUv = material.GetMetallicRoughnessUvMap();
    emissiveUv = material.GetEmissiveUvMap();
    iridescenceThicknessUv = material.GetIridescenceThicknessUvMap();
    normalScale = material.GetNormalScale();
}

void MaterialAsset::ApplyTo(MaterialComponent& material) const {
    material.SetBaseColorTexture(baseColor);
    material.SetNormalTexture(normalMap);
    material.SetMetallicRoughnessTexture(metallicRoughness);
    material.SetEmissiveTexture(emissiveMap);
    material.SetIridescenceThicknessTexture(iridescenceThicknessMap);
    if (metallicRoughness) {
        material.SetMetallic(1.0F);
        material.SetRoughness(1.0F);
        material.SetMetallicFactor(metallicFactor);
        material.SetRoughnessFactor(roughnessFactor);
    } else {
        material.SetMetallic(metallic);
        material.SetRoughness(roughness);
        material.SetMetallicFactor(metallicFactor);
        material.SetRoughnessFactor(roughnessFactor);
    }
    material.SetTint(tint);
    material.SetOcclusionStrength(occlusionStrength);
    material.SetEmissive(emissiveColor, emissiveIntensity);
    material.SetEmissiveFactor(emissiveFactor);
    material.SetShadingModel(shadingModel);
    material.SetToonDiffuseBands(toonDiffuseBands);
    material.SetToonRimIntensity(toonRimIntensity);
    material.SetToonRimPower(toonRimPower);
    material.SetDoubleSided(doubleSided);
    material.SetOpacity(opacity);
    material.SetAlphaCutoff(alphaCutoff);
    material.SetAlphaBlend(alphaBlend);
    material.SetGltfExtensions(gltfExtensions);
    material.SetBaseColorUvMap(baseColorUv);
    material.SetNormalUvMap(normalUv);
    material.SetMetallicRoughnessUvMap(metallicRoughnessUv);
    material.SetEmissiveUvMap(emissiveUv);
    material.SetIridescenceThicknessUvMap(iridescenceThicknessUv);
    material.SetNormalScale(normalScale);
}

void MaterialAsset::ApplyTo(MultiMaterialComponent::Slot& slot) const {
    slot.baseColor = baseColor;
    slot.normalMap = normalMap;
    slot.metallicRoughness = metallicRoughness;
    slot.emissiveMap = emissiveMap;
    slot.iridescenceThicknessMap = iridescenceThicknessMap;
    slot.tint = tint;
    if (metallicRoughness) {
        slot.metallic = 1.0F;
        slot.roughness = 1.0F;
        slot.metallicFactor = metallicFactor;
        slot.roughnessFactor = roughnessFactor;
    } else {
        slot.metallic = metallic;
        slot.roughness = roughness;
        slot.metallicFactor = metallicFactor;
        slot.roughnessFactor = roughnessFactor;
    }
    slot.occlusionStrength = occlusionStrength;
    slot.emissiveColor = emissiveColor;
    slot.emissiveIntensity = emissiveIntensity;
    slot.emissiveFactor = emissiveFactor;
    slot.shadingModel = shadingModel;
    slot.toonDiffuseBands = toonDiffuseBands;
    slot.toonRimIntensity = toonRimIntensity;
    slot.toonRimPower = toonRimPower;
    slot.doubleSided = doubleSided;
    slot.opacity = opacity;
    slot.alphaCutoff = alphaCutoff;
    slot.alphaBlend = alphaBlend;
    slot.gltfExtensions = gltfExtensions;
    slot.baseColorUv = baseColorUv;
    slot.normalUv = normalUv;
    slot.metallicRoughnessUv = metallicRoughnessUv;
    slot.emissiveUv = emissiveUv;
    slot.iridescenceThicknessUv = iridescenceThicknessUv;
    slot.normalScale = normalScale;
}

bool MaterialAsset::ApproximatelyEquals(const MaterialComponent& material) const noexcept {
    if (!SharedPtrEqual(baseColor, material.GetBaseColorTexture()) ||
        !SharedPtrEqual(normalMap, material.GetNormalTexture()) ||
        !SharedPtrEqual(metallicRoughness, material.GetMetallicRoughnessTexture()) ||
        !SharedPtrEqual(emissiveMap, material.GetEmissiveTexture())) {
        return false;
    }
    if (!ApproximatelyEqual(tint, material.GetTint()) ||
        !ApproximatelyEqual(metallic, material.GetMetallic()) ||
        !ApproximatelyEqual(roughness, material.GetRoughness()) ||
        !ApproximatelyEqual(metallicFactor, material.GetMetallicFactor()) ||
        !ApproximatelyEqual(roughnessFactor, material.GetRoughnessFactor()) ||
        !ApproximatelyEqual(occlusionStrength, material.GetOcclusionStrength()) ||
        !ApproximatelyEqual(emissiveColor, material.GetEmissiveColor()) ||
        !ApproximatelyEqual(emissiveIntensity, material.GetEmissiveIntensity()) ||
        !ApproximatelyEqual(emissiveFactor, material.GetEmissiveFactor()) ||
        material.GetShadingModel() != shadingModel ||
        material.GetToonDiffuseBands() != toonDiffuseBands ||
        !ApproximatelyEqual(toonRimIntensity, material.GetToonRimIntensity()) ||
        !ApproximatelyEqual(toonRimPower, material.GetToonRimPower()) ||
        material.IsDoubleSided() != doubleSided ||
        !ApproximatelyEqual(opacity, material.GetOpacity()) ||
        !ApproximatelyEqual(alphaCutoff, material.GetAlphaCutoff())) {
        return false;
    }
    const MaterialGltfExtensions& ext = material.GetGltfExtensions();
    if (!ApproximatelyEqual(gltfExtensions.clearcoatFactor, ext.clearcoatFactor) ||
        !ApproximatelyEqual(gltfExtensions.clearcoatRoughnessFactor, ext.clearcoatRoughnessFactor) ||
        !ApproximatelyEqual(gltfExtensions.transmissionFactor, ext.transmissionFactor) ||
        !ApproximatelyEqual(gltfExtensions.emissiveStrength, ext.emissiveStrength)) {
        return false;
    }
    return true;
}

}  // namespace Spark
