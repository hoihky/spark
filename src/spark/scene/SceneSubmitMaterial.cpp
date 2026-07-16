#include "spark/scene/SceneSubmit.hpp"

#include "spark/ecs/components/MaterialComponent.hpp"
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
    if (params.sceneTextures.GetSize() >= SceneRenderParams::MaxSceneTextures) {
        std::fprintf(
                stderr,
                "Spark: scene texture limit (%u) reached; dropping \"%s\"\n",
                SceneRenderParams::MaxSceneTextures,
                tex->GetName().CStr());
        return -1;
    }
    params.sceneTextures.PushBack(tex);
    return static_cast<std::int32_t>(params.sceneTextures.GetSize() - 1U);
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

}  // namespace Spark
