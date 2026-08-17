#include "spark/scene/SceneSubmit.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdio>

namespace Spark {

namespace {

void ApplyMaterialComponentToSceneDrawItemImpl(SceneDrawItem& item, const MaterialComponent* mat) noexcept {
    if (mat == nullptr) {
        return;
    }
    item.metallic = mat->GetMetallic();
    item.roughness = mat->GetRoughness();
    item.metallicFactor = mat->GetMetallicFactor();
    item.roughnessFactor = mat->GetRoughnessFactor();
    item.occlusionStrength = mat->GetOcclusionStrength();
    item.emissiveColor = mat->GetEmissiveColor();
    item.emissiveIntensity = mat->GetEmissiveIntensity();
    item.emissiveFactor = mat->GetEmissiveFactor();
    item.shadingModel = mat->GetShadingModel();
    item.toonDiffuseBands = mat->GetToonDiffuseBands();
    item.toonRimIntensity = mat->GetToonRimIntensity();
    item.toonRimPower = mat->GetToonRimPower();
    item.doubleSided = mat->IsDoubleSided();
    item.opacity = mat->GetOpacity();
}

}  // namespace

namespace SceneSubmitDetail {

std::int32_t FindOrAddSceneTexture(SceneRenderParams& params, const SharedPtr<Texture2D>& tex) {
    if (!tex) {
        return -1;
    }
    for (std::size_t i = 0; i < params.sceneTextures.GetSize(); ++i) {
        if (params.sceneTextures[i].Get() == tex.Get()) {
            return static_cast<std::int32_t>(i);
        }
    }
    const std::uint64_t fingerprint = tex->GetContentFingerprint();
    if (fingerprint != 0U) {
        for (std::size_t i = 0; i < params.sceneTextures.GetSize(); ++i) {
            const SharedPtr<Texture2D>& existing = params.sceneTextures[i];
            if (existing && existing->GetContentFingerprint() == fingerprint) {
                return static_cast<std::int32_t>(i);
            }
        }
    }
    if (params.sceneTextures.GetSize() >= SceneRenderParams::MaxSceneTextures) {
        std::fprintf(
                stderr,
                "Spark: scene texture limit (%u) reached; dropping \"%s\"\n",
                SceneRenderParams::MaxSceneTextures,
                tex->GetName().CStr());
        return -1;
    }
    const std::int32_t layer = static_cast<std::int32_t>(params.sceneTextures.GetSize());
    params.sceneTextures.PushBack(tex);
    return layer;
}

void ResolveIblEnvironmentLayer(SceneRenderParams& params) noexcept {
    if (params.iblEnvironmentLayer >= 0) {
        return;
    }
    for (std::size_t i = 0; i < params.draws.GetSize(); ++i) {
        const SceneDrawItem& d = params.draws[i];
        if (d.skyMode != SceneSkyMode::None && d.textureLayer >= 0) {
            params.iblEnvironmentLayer = d.textureLayer;
            return;
        }
    }
}

}  // namespace SceneSubmitDetail

void ApplyMaterialComponentToSceneDrawItem(
        SceneDrawItem& item,
        const MaterialComponent* mat,
        SceneRenderParams* resolveTextures) noexcept {
    ApplyMaterialComponentToSceneDrawItemImpl(item, mat);
    if (mat == nullptr || resolveTextures == nullptr) {
        return;
    }
    item.normalMapLayer = -1;
    item.metallicRoughnessMapLayer = -1;
    item.emissiveMapLayer = -1;
    if (mat->GetNormalTexture()) {
        item.normalMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, mat->GetNormalTexture());
    }
    if (mat->GetMetallicRoughnessTexture()) {
        item.metallicRoughnessMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, mat->GetMetallicRoughnessTexture());
    }
    if (mat->GetEmissiveTexture()) {
        item.emissiveMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, mat->GetEmissiveTexture());
    }
}

void ApplyMultiMaterialSlotToSceneDrawItem(
        SceneDrawItem& item,
        const MultiMaterialComponent::Slot& slot,
        SceneRenderParams* resolveTextures) noexcept {
    item.metallic = slot.metallic;
    item.roughness = slot.roughness;
    item.metallicFactor = slot.metallicFactor;
    item.roughnessFactor = slot.roughnessFactor;
    item.occlusionStrength = slot.occlusionStrength;
    item.emissiveColor = slot.emissiveColor;
    item.emissiveIntensity = slot.emissiveIntensity;
    item.emissiveFactor = slot.emissiveFactor;
    item.shadingModel = slot.shadingModel;
    item.doubleSided = slot.doubleSided;
    item.opacity = slot.opacity;
    item.normalMapLayer = -1;
    item.metallicRoughnessMapLayer = -1;
    item.emissiveMapLayer = -1;
    if (resolveTextures == nullptr) {
        return;
    }
    if (slot.normalMap) {
        item.normalMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, slot.normalMap);
    }
    if (slot.metallicRoughness) {
        item.metallicRoughnessMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, slot.metallicRoughness);
    }
    if (slot.emissiveMap) {
        item.emissiveMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, slot.emissiveMap);
    }
}

}  // namespace Spark
