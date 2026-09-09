#include "spark/scene/submit/SkinnedPaletteResolver.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/animation/SkeletonPaletteCache.hpp"
#include "spark/animation/SkeletonPaletteCacheKey.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SkinnedAnimationBudget.hpp"
#include "spark/scene/submit/SkinnedIkService.hpp"

namespace Spark {

SkinnedPaletteResolver::SkinnedPaletteResolver(
        SkinnedAnimationBudget& inBudget,
        SkeletonPaletteCache& inPaletteCache) noexcept
        : budget(inBudget), paletteCache(inPaletteCache) {}

void SkinnedPaletteResolver::FillBindPose(
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteMax) const {
    if (animator != nullptr && animator->GetSkeleton()) {
        animator->GetSkeleton()->BuildBindPosePalette(outPalette.GetData(), paletteMax);
        return;
    }
    if (const SharedPtr<Skeleton>& skeleton = mesh.GetSkeleton()) {
        skeleton->BuildBindPosePalette(outPalette.GetData(), paletteMax);
    }
}

bool SkinnedPaletteResolver::TryResolveAnimatedPalette(
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteMax) {
    if (animator != nullptr && animator->GetSkeleton()) {
        cacheKey.CaptureFromAnimator(*animator, paletteCache.GetQuantizationHz());
        if (paletteCache.IsEnabled() && paletteCache.TryCopy(cacheKey, outPalette)) {
            budget.RecordPaletteCacheHit();
            return true;
        }

        if (budget.CanComputePalette() && budget.ConsumePaletteUpdate()) {
            Array<Transform> pose;
            if (animator->TrySampleEvaluatedPose(pose)) {
                animator->GetSkeleton()->BuildPaletteFromPose(pose, outPalette.GetData(), paletteMax);
                paletteCache.Store(cacheKey, outPalette);
                return true;
            }
            FillBindPose(mesh, animator, outPalette, paletteMax);
            budget.RecordBindPoseFallback();
            return true;
        }

        FillBindPose(mesh, animator, outPalette, paletteMax);
        budget.RecordBindPoseFallback();
        return true;
    }

    FillBindPose(mesh, animator, outPalette, paletteMax);
    return true;
}

bool SkinnedPaletteResolver::TryResolve(
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteMax) {
    if (paletteMax == 0 || !mesh.GetMesh()) {
        return false;
    }
    if (!budget.CanSubmitSkinnedDraw()) {
        return false;
    }

    const std::uint32_t jointCount = animator != nullptr && animator->GetSkeleton()
            ? animator->GetSkeleton()->GetJointCount()
            : (mesh.GetSkeleton() ? mesh.GetSkeleton()->GetJointCount() : 0U);
    if (jointCount == 0 || jointCount > paletteMax) {
        return false;
    }
    outPalette.Resize(jointCount);

    if (!TryResolveAnimatedPalette(mesh, animator, outPalette, paletteMax)) {
        return false;
    }
    return budget.ConsumeSkinnedDraw();
}

bool SkinnedPaletteResolver::TryResolve(
        GameObject* owner,
        const Matrix4& ownerWorld,
        GameWorld& world,
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteMax) {
    if (owner == nullptr || !SkinnedIkService::ObjectUsesIk(*owner)) {
        return TryResolve(mesh, animator, outPalette, paletteMax);
    }

    if (paletteMax == 0 || !mesh.GetMesh()) {
        return false;
    }
    if (!budget.CanSubmitSkinnedDraw()) {
        return false;
    }

    const std::uint32_t jointCount = animator != nullptr && animator->GetSkeleton()
            ? animator->GetSkeleton()->GetJointCount()
            : (mesh.GetSkeleton() ? mesh.GetSkeleton()->GetJointCount() : 0U);
    if (jointCount == 0 || jointCount > paletteMax) {
        return false;
    }
    outPalette.Resize(jointCount);

    if (animator == nullptr || !animator->GetSkeleton()) {
        FillBindPose(mesh, animator, outPalette, paletteMax);
        return budget.ConsumeSkinnedDraw();
    }

    if (!budget.CanComputePalette() || !budget.ConsumePaletteUpdate()) {
        FillBindPose(mesh, animator, outPalette, paletteMax);
        budget.RecordBindPoseFallback();
        return budget.ConsumeSkinnedDraw();
    }

    Array<Transform> pose;
    if (!animator->TrySampleEvaluatedPose(pose)) {
        FillBindPose(mesh, animator, outPalette, paletteMax);
        return budget.ConsumeSkinnedDraw();
    }

    world.GetSkinnedIkService().ApplyToPose(*owner, ownerWorld, *animator, pose);
    animator->GetSkeleton()->BuildPaletteFromPose(pose, outPalette.GetData(), paletteMax);
    return budget.ConsumeSkinnedDraw();
}

}  // namespace Spark
