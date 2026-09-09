#include "spark/scene/submit/SkinnedPaletteResolver.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/animation/SkeletonPaletteCache.hpp"
#include "spark/animation/SkeletonPaletteCacheKey.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/submit/SkinnedAnimationBudget.hpp"

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

    if (animator != nullptr && animator->GetSkeleton()) {
        cacheKey.CaptureFromAnimator(*animator, paletteCache.GetQuantizationHz());
        if (paletteCache.IsEnabled() && paletteCache.TryCopy(cacheKey, outPalette)) {
            budget.RecordPaletteCacheHit();
            return budget.ConsumeSkinnedDraw();
        }

        if (budget.CanComputePalette() && budget.ConsumePaletteUpdate()) {
            animator->ComputeJointPalette(outPalette.GetData(), paletteMax);
            paletteCache.Store(cacheKey, outPalette);
            return budget.ConsumeSkinnedDraw();
        }

        FillBindPose(mesh, animator, outPalette, paletteMax);
        budget.RecordBindPoseFallback();
        return budget.ConsumeSkinnedDraw();
    }

    FillBindPose(mesh, animator, outPalette, paletteMax);
    return budget.ConsumeSkinnedDraw();
}

}  // namespace Spark
