#include "spark/scene/submit/SceneRigidMeshDrawBuilder.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/SkinnedSceneDrawMaterialApplicator.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

SceneRigidMeshDrawBuilder::SceneRigidMeshDrawBuilder(
        const SceneSubmitDetail::FindSceneTextureFn& inFindTexture,
        SceneRenderParams& inParams) noexcept
    : findTexture(inFindTexture), params(inParams) {}

void SceneRigidMeshDrawBuilder::ApplyAlbedoTexture(
        SceneDrawItem& item,
        const SharedPtr<Texture2D>& baseColor,
        const Vector3& tint,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture) {
    item.albedo = {item.albedo.x * tint.x, item.albedo.y * tint.y, item.albedo.z * tint.z};
    if (!baseColor) {
        return;
    }
    bool isHdr = false;
    item.textureLayer = findOrAddTexture(baseColor, &item.textureUvScale, &item.textureUvOffset);
    if (baseColor) {
        Vector2 uvScale{1.0F, 1.0F};
        Vector2 uvOffset{};
        SharedPtr<Texture2D> resolved = baseColor->ResolveAtlasUv(uvScale, uvOffset);
        if (!resolved) {
            resolved = baseColor;
        }
        const Vector2 layerScale = resolved->GetSceneLayerUvScale();
        item.textureUvScale.x *= layerScale.x;
        item.textureUvScale.y *= layerScale.y;
        isHdr = resolved->IsHdrFloatPixels();
    }
    item.textureIsHdrLinear = isHdr && item.textureLayer >= 0;
}

void SceneRigidMeshDrawBuilder::AppendRigidMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const Mesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat) {
    const Array<MeshSubmesh>& submeshes = mesh.GetSubmeshes();
    if (submeshes.IsEmpty() || multiMat == nullptr) {
        SceneDrawItem item = baseItem;
        item.submeshIndex = kSceneDrawFullSubmesh;
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
            ApplyAlbedoTexture(item, mat->GetBaseColorTexture(), mat->GetTint(), findTexture);
        } else if (multiMat != nullptr && multiMat->GetSlotCount() > 0U) {
            const MultiMaterialComponent::Slot& slot = multiMat->GetSlot(0U);
            ApplyMultiMaterialSlotToSceneDrawItem(item, slot, &params);
            ApplyAlbedoTexture(item, slot.baseColor, slot.tint, findTexture);
        }
        drawList.PushBack(item);
        return;
    }

    for (std::size_t si = 0; si < submeshes.GetSize(); ++si) {
        SceneDrawItem item = baseItem;
        item.submeshIndex = static_cast<std::uint32_t>(si);
        const MeshSubmesh& sm = submeshes[si];
        const std::uint32_t materialIndex = sm.ResolveMaterialIndex(multiMat->GetActiveVariantIndex());
        if (materialIndex < multiMat->GetSlotCount()) {
            const MultiMaterialComponent::Slot& slot = multiMat->GetSlot(materialIndex);
            ApplyMultiMaterialSlotToSceneDrawItem(item, slot, &params);
            ApplyAlbedoTexture(item, slot.baseColor, slot.tint, findTexture);
        } else if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
            ApplyAlbedoTexture(item, mat->GetBaseColorTexture(), mat->GetTint(), findTexture);
        }
        drawList.PushBack(item);
    }
}

namespace SceneSubmitDetail {

void ApplyAlbedoTexture(
        SceneDrawItem& item,
        const SharedPtr<Texture2D>& baseColor,
        const Vector3& tint,
        const FindSceneTextureFn& findOrAddTexture) {
    SceneRigidMeshDrawBuilder::ApplyAlbedoTexture(item, baseColor, tint, findOrAddTexture);
}

void PushRigidMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const Mesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const FindSceneTextureFn& findOrAddTexture) {
    SceneRigidMeshDrawBuilder builder(findOrAddTexture, params);
    builder.AppendRigidMeshDraws(drawList, baseItem, mesh, mat, multiMat);
}

void PushSkinnedMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const SkinnedMesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const FindSceneTextureFn& findOrAddTexture) {
    SkinnedSceneDrawMaterialApplicator applicator(params, findOrAddTexture);
    applicator.AppendSkinnedDraws(drawList, baseItem, mesh, mat, multiMat);
}

}  // namespace SceneSubmitDetail

}  // namespace Spark