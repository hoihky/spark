#include "spark/scene/submit/SkinnedSceneDrawMaterialApplicator.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/mesh/MeshSubmesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

SkinnedSceneDrawMaterialApplicator::TextureLayerResolver::TextureLayerResolver(
        SceneRenderParams& inRenderParams,
        const SceneSubmitDetail::FindSceneTextureFn& inFindTexture)
        : renderParams(inRenderParams), findTexture(inFindTexture) {}

std::int32_t SkinnedSceneDrawMaterialApplicator::TextureLayerResolver::Resolve(
        const SharedPtr<Texture2D>& texture,
        Vector2* uvScale,
        Vector2* uvOffset) const {
    return findTexture(texture, uvScale, uvOffset);
}

SkinnedSceneDrawMaterialApplicator::SkinnedSceneDrawMaterialApplicator(
        SceneRenderParams& inRenderParams,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture)
        : renderParams(inRenderParams), textureResolver(inRenderParams, findTexture) {}

void SkinnedSceneDrawMaterialApplicator::ApplyMaterial(SceneDrawItem& item, const MaterialComponent& material) {
    ApplyMaterialComponentToSceneDrawItem(item, &material, &renderParams);
    item.textureLayer = -1;
    item.normalMapLayer = -1;
    item.metallicRoughnessMapLayer = -1;
    item.emissiveMapLayer = -1;
    item.iridescenceThicknessMapLayer = -1;
    SceneSubmitDetail::ApplyAlbedoTexture(
            item, material.GetBaseColorTexture(), material.GetTint(), textureResolver.GetFindTexture());
    if (material.GetNormalTexture()) {
        item.normalMapLayer = textureResolver.Resolve(material.GetNormalTexture());
    }
    if (material.GetMetallicRoughnessTexture()) {
        item.metallicRoughnessMapLayer =
                textureResolver.Resolve(material.GetMetallicRoughnessTexture());
    }
    if (material.GetEmissiveTexture()) {
        item.emissiveMapLayer = textureResolver.Resolve(material.GetEmissiveTexture());
    }
    if (material.GetIridescenceThicknessTexture()) {
        item.iridescenceThicknessMapLayer =
                textureResolver.Resolve(material.GetIridescenceThicknessTexture());
    }
}

void SkinnedSceneDrawMaterialApplicator::ApplySlot(
        SceneDrawItem& item,
        const MultiMaterialComponent::Slot& slot) {
    ApplyMultiMaterialSlotToSceneDrawItem(item, slot, &renderParams);
    item.textureLayer = -1;
    item.normalMapLayer = -1;
    item.metallicRoughnessMapLayer = -1;
    item.emissiveMapLayer = -1;
    item.iridescenceThicknessMapLayer = -1;
    SceneSubmitDetail::ApplyAlbedoTexture(item, slot.baseColor, slot.tint, textureResolver.GetFindTexture());
    if (slot.normalMap) {
        item.normalMapLayer = textureResolver.Resolve(slot.normalMap);
    }
    if (slot.metallicRoughness) {
        item.metallicRoughnessMapLayer = textureResolver.Resolve(slot.metallicRoughness);
    }
    if (slot.emissiveMap) {
        item.emissiveMapLayer = textureResolver.Resolve(slot.emissiveMap);
    }
    if (slot.iridescenceThicknessMap) {
        item.iridescenceThicknessMapLayer = textureResolver.Resolve(slot.iridescenceThicknessMap);
    }
}

void SkinnedSceneDrawMaterialApplicator::AppendSkinnedDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const SkinnedMesh& mesh,
        const MaterialComponent* material,
        const MultiMaterialComponent* multiMaterial) {
    const Array<MeshSubmesh>& submeshes = mesh.GetSubmeshes();
    if (submeshes.IsEmpty() || multiMaterial == nullptr) {
        SceneDrawItem item = baseItem;
        item.submeshIndex = kSceneDrawFullSubmesh;
        if (material != nullptr) {
            ApplyMaterial(item, *material);
        } else if (multiMaterial != nullptr && multiMaterial->GetSlotCount() > 0U) {
            ApplySlot(item, multiMaterial->GetSlot(0U));
        }
        drawList.PushBack(item);
        return;
    }

    for (std::size_t submeshIndex = 0; submeshIndex < submeshes.GetSize(); ++submeshIndex) {
        SceneDrawItem item = baseItem;
        item.submeshIndex = static_cast<std::uint32_t>(submeshIndex);
        const MeshSubmesh& submesh = submeshes[submeshIndex];
        const std::uint32_t materialIndex = submesh.ResolveMaterialIndex(multiMaterial->GetActiveVariantIndex());
        if (materialIndex < multiMaterial->GetSlotCount()) {
            ApplySlot(item, multiMaterial->GetSlot(materialIndex));
        } else if (material != nullptr) {
            ApplyMaterial(item, *material);
        }
        drawList.PushBack(item);
    }
}

}  // namespace Spark
