#include "spark/scene/submit/detail/SkinnedMeshPalette.hpp"

#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"

namespace Spark {

bool SkinnedMeshPalette::TryFill(
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Matrix4* outPalette,
        const std::uint32_t paletteMax) noexcept {
    if (outPalette == nullptr || paletteMax == 0 || !mesh.GetMesh()) {
        return false;
    }
    if (animator != nullptr && animator->GetSkeleton()) {
        animator->ComputeJointPalette(outPalette, paletteMax);
        return true;
    }
    if (const SharedPtr<Skeleton>& skeleton = mesh.GetSkeleton()) {
        skeleton->BuildBindPosePalette(outPalette, paletteMax);
        return skeleton->GetJointCount() > 0;
    }
    return false;
}

}  // namespace Spark
