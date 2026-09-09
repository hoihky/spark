#include "spark/scene/submit/SkinnedAnimationService.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/animation/SkinnedAnimationBudgetComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/math/Matrix4.hpp"

namespace Spark {

void SkinnedAnimationService::BeginSubmitFrame(const std::uint32_t frameIndex) noexcept {
    submitFrameIndex = frameIndex;
    budget.BeginFrame();
    paletteCache.BeginFrame(frameIndex);
}

void SkinnedAnimationService::ApplyBudgetPolicy(const SkinnedAnimationBudgetComponent& policy) noexcept {
    budget.SetMaxPaletteUpdatesPerFrame(policy.GetMaxPaletteUpdatesPerFrame());
    budget.SetMaxSkinnedDrawsPerFrame(policy.GetMaxSkinnedDrawsPerFrame());
    paletteCache.SetEnabled(policy.IsPaletteCacheEnabled());
    paletteCache.SetQuantizationHz(policy.GetPaletteQuantizationHz());
}

void SkinnedAnimationService::ResetPolicyToDefaults() noexcept {
    budget.SetMaxPaletteUpdatesPerFrame(0);
    budget.SetMaxSkinnedDrawsPerFrame(0);
    paletteCache.SetEnabled(true);
    paletteCache.SetQuantizationHz(30.0F);
}

bool SkinnedAnimationService::TryResolvePalette(
        const SkinnedMeshComponent& mesh,
        const AnimatorComponent* animator,
        Array<Matrix4>& outPalette,
        const std::uint32_t paletteMax) {
    return resolver.TryResolve(mesh, animator, outPalette, paletteMax);
}

}  // namespace Spark
