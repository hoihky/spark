#include "spark/scene/submit/SceneSubmit.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/scene/texture/Texture2D.hpp"

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
    item.alphaCutoff = mat->GetAlphaCutoff();
    item.alphaBlend = mat->IsAlphaBlend();
    item.baseColorUv = mat->GetBaseColorUvMap();
    item.normalUv = mat->GetNormalUvMap();
    item.metallicRoughnessUv = mat->GetMetallicRoughnessUvMap();
    item.emissiveUv = mat->GetEmissiveUvMap();
    item.iridescenceThicknessUv = mat->GetIridescenceThicknessUvMap();
    item.normalScale = mat->GetNormalScale();
    item.gltfExtensions = mat->GetGltfExtensions();
}

}  // namespace

namespace {

void ApplyAtlasUvToMap(MaterialUvMap& map, const SharedPtr<Texture2D>& tex) noexcept {
    if (!tex || !tex->HasAtlasBinding()) {
        return;
    }
    Vector2 uvScale{1.0F, 1.0F};
    Vector2 uvOffset{};
    (void)tex->ResolveAtlasUv(uvScale, uvOffset);
    map.uvScale.x *= uvScale.x;
    map.uvScale.y *= uvScale.y;
    map.uvOffset.x += uvOffset.x;
    map.uvOffset.y += uvOffset.y;
}

}  // namespace

namespace SceneSubmitDetail {

std::int32_t FindOrAddSceneTexture(
        SceneRenderParams& params,
        const SharedPtr<Texture2D>& tex,
        Vector2* outUvScale,
        Vector2* outUvOffset,
        bool* outIsHdrLinear) {
    if (outIsHdrLinear != nullptr) {
        *outIsHdrLinear = false;
    }
    if (!tex) {
        return -1;
    }
    Vector2 uvScale{1.0F, 1.0F};
    Vector2 uvOffset{};
    SharedPtr<Texture2D> resolved = tex->ResolveAtlasUv(uvScale, uvOffset);
    if (!resolved) {
        resolved = tex;
    }
    if (outUvScale != nullptr) {
        *outUvScale = uvScale;
    }
    if (outUvOffset != nullptr) {
        *outUvOffset = uvOffset;
    }

    const bool isHdr = resolved->IsHdrFloatPixels();
    if (outIsHdrLinear != nullptr) {
        *outIsHdrLinear = isHdr;
    }

    Array<SharedPtr<Texture2D>>& targetArray =
            isHdr ? params.sceneHdrTextures : params.sceneTextures;
    const std::uint32_t maxLayers =
            isHdr ? SceneRenderParams::MaxSceneHdrTextures : SceneRenderParams::MaxSceneTextures;

    for (std::size_t i = 0; i < targetArray.GetSize(); ++i) {
        if (targetArray[i].Get() == resolved.Get()) {
            return static_cast<std::int32_t>(i);
        }
    }

    if (targetArray.GetSize() >= maxLayers) {
        std::fprintf(
                stderr,
                "Spark: scene %s texture limit (%u) reached; dropping \"%s\"\n",
                isHdr ? "HDR" : "LDR",
                maxLayers,
                resolved->GetName().CStr());
        if (outIsHdrLinear != nullptr) {
            *outIsHdrLinear = false;
        }
        return -1;
    }
    const std::int32_t layer = static_cast<std::int32_t>(targetArray.GetSize());
    targetArray.PushBack(resolved);
    return layer;
}

void ResolveIblEnvironmentLayer(SceneRenderParams& params) noexcept {
    params.iblEnvironmentUvScale = {1.0F, 1.0F};
    if (params.iblEnvironmentLayer >= 0) {
        const std::size_t idx = static_cast<std::size_t>(params.iblEnvironmentLayer);
        if (params.iblEnvironmentIsHdr) {
            if (idx < params.sceneHdrTextures.GetSize() && params.sceneHdrTextures[idx]) {
                params.iblEnvironmentUvScale = params.sceneHdrTextures[idx]->GetSceneLayerUvScale();
            }
        } else if (idx < params.sceneTextures.GetSize() && params.sceneTextures[idx]) {
            params.iblEnvironmentUvScale = params.sceneTextures[idx]->GetSceneLayerUvScale();
        }
        return;
    }
    params.iblEnvironmentIsHdr = false;
    for (std::size_t i = 0; i < params.draws.GetSize(); ++i) {
        const SceneDrawItem& d = params.draws[i];
        if (d.skyMode != SceneSkyMode::None && d.textureLayer >= 0) {
            if (d.textureIsHdrLinear && !params.iblUseHdrSkyEnvironment) {
                continue;
            }
            params.iblEnvironmentLayer = d.textureLayer;
            params.iblEnvironmentIsHdr = d.textureIsHdrLinear;
            params.iblEnvironmentUvScale = d.textureUvScale;
            return;
        }
    }
    params.iblEnvironmentLayer = -1;
}

void PopulateSkyDrawItem(
        SceneDrawItem& item,
        const SkyComponent& sky,
        const MeshComponent& mc,
        const MaterialComponent* mat,
        const Matrix4& worldM,
        SceneRenderParams& params) noexcept {
    item.mesh = SceneMeshSlot::Custom;
    item.skyMode = sky.GetSkyMode();
    item.model = worldM;
    item.customMesh = mc.GetMesh();
    item.albedo = sky.GetTint();
    item.textureLayer = -1;
    item.textureIsHdrLinear = false;
    item.metallic = 0.0F;
    item.roughness = 1.0F;
    item.shadowFlags = 0;
    if (mat != nullptr && mat->GetBaseColorTexture()) {
        const Vector3& t = mat->GetTint();
        item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
        bool isHdr = false;
        item.textureLayer = FindOrAddSceneTexture(params, mat->GetBaseColorTexture(), nullptr, nullptr, &isHdr);
        Vector2 atlasScale{1.0F, 1.0F};
        Vector2 atlasOffset{};
        SharedPtr<Texture2D> resolved = mat->GetBaseColorTexture()->ResolveAtlasUv(atlasScale, atlasOffset);
        if (!resolved) {
            resolved = mat->GetBaseColorTexture();
        }
        item.textureUvScale = resolved->GetSceneLayerUvScale();
        item.textureUvOffset = {};
        item.textureIsHdrLinear = isHdr && item.textureLayer >= 0;
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
    item.iridescenceThicknessMapLayer = -1;
    if (mat->GetNormalTexture()) {
        item.normalMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, mat->GetNormalTexture(), nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.normalUv, mat->GetNormalTexture());
    }
    if (mat->GetMetallicRoughnessTexture()) {
        item.metallicRoughnessMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(
                *resolveTextures, mat->GetMetallicRoughnessTexture(), nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.metallicRoughnessUv, mat->GetMetallicRoughnessTexture());
    }
    if (mat->GetEmissiveTexture()) {
        item.emissiveMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, mat->GetEmissiveTexture(), nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.emissiveUv, mat->GetEmissiveTexture());
    }
    if (mat->GetIridescenceThicknessTexture()) {
        item.iridescenceThicknessMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(
                *resolveTextures, mat->GetIridescenceThicknessTexture(), nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.iridescenceThicknessUv, mat->GetIridescenceThicknessTexture());
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
    item.toonDiffuseBands = slot.toonDiffuseBands;
    item.toonRimIntensity = slot.toonRimIntensity;
    item.toonRimPower = slot.toonRimPower;
    item.doubleSided = slot.doubleSided;
    item.opacity = slot.opacity;
    item.alphaCutoff = slot.alphaCutoff;
    item.alphaBlend = slot.alphaBlend;
    item.baseColorUv = slot.baseColorUv;
    item.normalUv = slot.normalUv;
    item.metallicRoughnessUv = slot.metallicRoughnessUv;
    item.emissiveUv = slot.emissiveUv;
    item.iridescenceThicknessUv = slot.iridescenceThicknessUv;
    item.normalScale = slot.normalScale;
    item.gltfExtensions = slot.gltfExtensions;
    item.normalMapLayer = -1;
    item.metallicRoughnessMapLayer = -1;
    item.emissiveMapLayer = -1;
    item.iridescenceThicknessMapLayer = -1;
    if (resolveTextures == nullptr) {
        return;
    }
    if (slot.normalMap) {
        item.normalMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, slot.normalMap, nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.normalUv, slot.normalMap);
    }
    if (slot.metallicRoughness) {
        item.metallicRoughnessMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(
                *resolveTextures, slot.metallicRoughness, nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.metallicRoughnessUv, slot.metallicRoughness);
    }
    if (slot.emissiveMap) {
        item.emissiveMapLayer =
                SceneSubmitDetail::FindOrAddSceneTexture(*resolveTextures, slot.emissiveMap, nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.emissiveUv, slot.emissiveMap);
    }
    if (slot.iridescenceThicknessMap) {
        item.iridescenceThicknessMapLayer = SceneSubmitDetail::FindOrAddSceneTexture(
                *resolveTextures, slot.iridescenceThicknessMap, nullptr, nullptr, nullptr);
        ApplyAtlasUvToMap(item.iridescenceThicknessUv, slot.iridescenceThicknessMap);
    }
}

}  // namespace Spark
