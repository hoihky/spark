#include "spark/scene/submit/SceneMeshSubmitCollectors.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"
#include "spark/scene/submit/SceneRigidMeshDrawBuilder.hpp"
#include "spark/ecs/components/rendering/MeshLodComponent.hpp"
#include "spark/scene/lod/MeshLodSubmit.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
namespace Spark {

RigidDrawableSubmitCollector::RigidDrawableSubmitCollector(
        Array<SceneDrawItem>& inDrawList,
        SceneRenderParams& inParams,
        const SceneSubmitDetail::FindSceneTextureFn& inFindTexture,
        const std::int32_t inDefaultShadowFlags,
        const bool inSkiesSubmittedOutsideCull,
        const Vector3& inCameraPositionWorld) noexcept
    : drawList(inDrawList),
      params(inParams),
      findTexture(inFindTexture),
      defaultShadowFlags(inDefaultShadowFlags),
      skiesSubmittedOutsideCull(inSkiesSubmittedOutsideCull),
      cameraPositionWorld(inCameraPositionWorld) {}

void RigidDrawableSubmitCollector::OnDrawable(
        GameObject* object,
        const MeshComponent& mc,
        const MaterialComponent* mat,
        const Matrix4& worldM) {
    if (object == nullptr) {
        return;
    }
    if (!mc.GetMesh() && mc.GetSlot() == SceneMeshSlot::Custom) {
        return;
    }
    MeshLodComponent* lod = object != nullptr ? object->GetComponent<MeshLodComponent>() : nullptr;
    const MeshLodResolvedDraw resolved =
            MeshLodResolveRigidDraw(mc, lod, cameraPositionWorld, worldM);
    if (resolved.culled) {
        return;
    }
    const MultiMaterialComponent* multiMat = object->GetComponent<MultiMaterialComponent>();
    if (object->GetComponent<WaterBodyComponent>() != nullptr) {
        return;
    }
    const SkyComponent* sky = object->GetComponent<SkyComponent>();
    if (sky != nullptr && sky->IsSkyEnabled()) {
        if (skiesSubmittedOutsideCull) {
            return;
        }
        SceneDrawItem item{};
        SceneSubmitDetail::PopulateSkyDrawItem(item, *sky, mc, mat, worldM, params);
        DrawableSortResolver::ApplyToSceneDrawItem(*object, item, 0);
        drawList.PushBack(item);
        return;
    }
    SceneDrawItem baseItem{};
    baseItem.model = worldM;
    baseItem.mesh = resolved.meshSlot;
    baseItem.albedo = mc.GetAlbedo();
    baseItem.textureLayer = -1;
    if (resolved.meshSlot == SceneMeshSlot::Custom) {
        baseItem.customMesh = resolved.rigidMesh;
        if (!baseItem.customMesh) {
            return;
        }
    }
    if (resolved.meshSlot == SceneMeshSlot::GroundPlane) {
        baseItem.doubleSided = true;
    }
    baseItem.shadowFlags = SceneSubmitDetail::ResolveDrawableShadowFlags(
            object, resolved.meshSlot, worldM, mat, defaultShadowFlags);
    DrawableSortResolver::ApplyToSceneDrawItem(*object, baseItem, 0);

    if (resolved.meshSlot == SceneMeshSlot::Custom && resolved.rigidMesh && multiMat != nullptr &&
        !resolved.rigidMesh->GetSubmeshes().IsEmpty()) {
        SceneRigidMeshDrawBuilder builder(findTexture, params);
        builder.AppendRigidMeshDraws(drawList, baseItem, *resolved.rigidMesh, mat, multiMat);
        return;
    }

    SceneDrawItem item = baseItem;
    if (mat != nullptr) {
        ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
        SceneRigidMeshDrawBuilder::ApplyAlbedoTexture(item, mat->GetBaseColorTexture(), mat->GetTint(), findTexture);
    }
    drawList.PushBack(item);
}

SkinnedDrawableSubmitCollector::SkinnedDrawableSubmitCollector(
        GameWorld& inGameWorld,
        Array<SceneDrawItem>& inDrawList,
        SceneRenderParams& inParams,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture,
        const std::int32_t inDefaultShadowFlags,
        const Vector3& inCameraPositionWorld) noexcept
    : gameWorld(inGameWorld),
      drawList(inDrawList),
      params(inParams),
      materialApplicator(inParams, findTexture),
      defaultShadowFlags(inDefaultShadowFlags),
      cameraPositionWorld(inCameraPositionWorld) {}

void SkinnedDrawableSubmitCollector::FillJointPalette(
        GameObject* owner,
        const Matrix4& ownerWorld,
        GameWorld& inGameWorld,
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteUpdateStride) {
    outPalette.Clear();
    (void)inGameWorld.GetSkinnedAnimationService().TryResolvePalette(
            owner,
            ownerWorld,
            inGameWorld,
            mesh,
            animator,
            outPalette,
            Skeleton::MaxJoints,
            paletteUpdateStride);
}

void SkinnedDrawableSubmitCollector::OnSkinnedDrawable(
        GameObject* object,
        const SkinnedMeshComponent& smc,
        const MaterialComponent* mat,
        const AnimatorComponent* anim,
        const Matrix4& world) {
    if (!smc.GetMesh()) {
        return;
    }
    MeshLodComponent* lod = object != nullptr ? object->GetComponent<MeshLodComponent>() : nullptr;
    const MeshLodResolvedDraw resolved =
            MeshLodResolveSkinnedDraw(smc, lod, cameraPositionWorld, world);
    if (resolved.culled || !resolved.skinnedMesh) {
        return;
    }
    const std::uint32_t jc = anim != nullptr && anim->GetSkeleton()
            ? anim->GetSkeleton()->GetJointCount()
            : (smc.GetSkeleton() ? smc.GetSkeleton()->GetJointCount() : 0U);
    if (jc == 0) {
        return;
    }
    const MultiMaterialComponent* multiMat =
            object != nullptr ? object->GetComponent<MultiMaterialComponent>() : nullptr;

    SceneDrawItem baseItem{};
    baseItem.model = world;
    baseItem.mesh = SceneMeshSlot::Custom;
    baseItem.skinnedMesh = resolved.skinnedMesh;
    baseItem.albedo = {0.9F, 0.88F, 0.82F};
    baseItem.textureLayer = -1;
    baseItem.metallic = 0.0F;
    baseItem.roughness = 0.5F;
    FillJointPalette(
            object, world, gameWorld, smc, anim, baseItem.jointPalette, resolved.paletteUpdateStride);
    if (baseItem.jointPalette.IsEmpty()) {
        return;
    }
    baseItem.shadowFlags = defaultShadowFlags;
    if (object != nullptr) {
        DrawableSortResolver::ApplyToSceneDrawItem(*object, baseItem, 0);
    }

    materialApplicator.AppendSkinnedDraws(drawList, baseItem, *resolved.skinnedMesh, mat, multiMat);
}

}  // namespace Spark