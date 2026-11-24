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
#include "spark/scene/submit/SceneRigidMeshDrawBuilder.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
namespace Spark {

RigidDrawableSubmitCollector::RigidDrawableSubmitCollector(
        Array<SceneDrawItem>& inDrawList,
        SceneRenderParams& inParams,
        const SceneSubmitDetail::FindSceneTextureFn& inFindTexture,
        const std::int32_t inDefaultShadowFlags,
        const bool inSkiesSubmittedOutsideCull) noexcept
    : drawList(inDrawList),
      params(inParams),
      findTexture(inFindTexture),
      defaultShadowFlags(inDefaultShadowFlags),
      skiesSubmittedOutsideCull(inSkiesSubmittedOutsideCull) {}

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
        drawList.PushBack(item);
        return;
    }
    SceneDrawItem baseItem{};
    baseItem.model = worldM;
    baseItem.mesh = mc.GetSlot();
    baseItem.albedo = mc.GetAlbedo();
    baseItem.textureLayer = -1;
    if (mc.GetSlot() == SceneMeshSlot::Custom) {
        baseItem.customMesh = mc.GetMesh();
    }
    if (mc.GetSlot() == SceneMeshSlot::GroundPlane) {
        baseItem.doubleSided = true;
    }
    baseItem.shadowFlags = SceneSubmitDetail::ResolveDrawableShadowFlags(
            object, mc.GetSlot(), worldM, mat, defaultShadowFlags);

    if (mc.GetSlot() == SceneMeshSlot::Custom && mc.GetMesh() && multiMat != nullptr &&
        !mc.GetMesh()->GetSubmeshes().IsEmpty()) {
        SceneRigidMeshDrawBuilder builder(findTexture, params);
        builder.AppendRigidMeshDraws(drawList, baseItem, *mc.GetMesh(), mat, multiMat);
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
        const std::int32_t inDefaultShadowFlags) noexcept
    : gameWorld(inGameWorld),
      drawList(inDrawList),
      params(inParams),
      materialApplicator(inParams, findTexture),
      defaultShadowFlags(inDefaultShadowFlags) {}

void SkinnedDrawableSubmitCollector::FillJointPalette(
        GameObject* owner,
        const Matrix4& ownerWorld,
        GameWorld& inGameWorld,
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette) {
    outPalette.Clear();
    (void)inGameWorld.GetSkinnedAnimationService().TryResolvePalette(
            owner, ownerWorld, inGameWorld, mesh, animator, outPalette, Skeleton::MaxJoints);
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
    baseItem.skinnedMesh = smc.GetMesh();
    baseItem.albedo = {0.9F, 0.88F, 0.82F};
    baseItem.textureLayer = -1;
    baseItem.metallic = 0.0F;
    baseItem.roughness = 0.5F;
    FillJointPalette(object, world, gameWorld, smc, anim, baseItem.jointPalette);
    if (baseItem.jointPalette.IsEmpty()) {
        return;
    }
    baseItem.shadowFlags = defaultShadowFlags;

    materialApplicator.AppendSkinnedDraws(drawList, baseItem, *smc.GetMesh(), mat, multiMat);
}

}  // namespace Spark