#pragma once

#include "spark/animation/SkeletonPaletteCacheKey.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class SkinnedAnimationBudget;
class SkinnedMeshComponent;
class SkeletonPaletteCache;

/**
 * Resolves joint palettes for scene submit using palette cache + frame budget (Strategy).
 */
class SkinnedPaletteResolver {
public:
    SkinnedPaletteResolver(SkinnedAnimationBudget& budget, SkeletonPaletteCache& paletteCache) noexcept;

    [[nodiscard]] bool TryResolve(
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Array<Matrix4>& outPalette,
            std::uint32_t paletteMax);

private:
    void FillBindPose(
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Array<Matrix4>& outPalette,
            std::uint32_t paletteMax) const;

    SkinnedAnimationBudget& budget;
    SkeletonPaletteCache& paletteCache;
    SkeletonPaletteCacheKey cacheKey{};
};

}  // namespace Spark
