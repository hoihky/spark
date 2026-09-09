#pragma once

#include "spark/animation/SkeletonPaletteCache.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/scene/submit/SkinnedAnimationBudget.hpp"
#include "spark/scene/submit/SkinnedPaletteResolver.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class GameObject;
class GameWorld;
class SkinnedAnimationBudgetComponent;
class SkinnedMeshComponent;

/**
 * Per-world skinned animation performance coordinator (cache + submit budget).
 */
class SkinnedAnimationService {
public:
    void BeginSubmitFrame(std::uint32_t frameIndex) noexcept;
    void ApplyBudgetPolicy(const SkinnedAnimationBudgetComponent& policy) noexcept;
    void ResetPolicyToDefaults() noexcept;

    [[nodiscard]] bool TryResolvePalette(
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Array<Matrix4>& outPalette,
            std::uint32_t paletteMax);

    [[nodiscard]] bool TryResolvePalette(
            GameObject* owner,
            const Matrix4& ownerWorld,
            GameWorld& world,
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Array<Matrix4>& outPalette,
            std::uint32_t paletteMax);

    [[nodiscard]] SkeletonPaletteCache& GetPaletteCache() noexcept { return paletteCache; }
    [[nodiscard]] const SkeletonPaletteCache& GetPaletteCache() const noexcept { return paletteCache; }

    [[nodiscard]] SkinnedAnimationBudget& GetBudget() noexcept { return budget; }
    [[nodiscard]] const SkinnedAnimationBudget& GetBudget() const noexcept { return budget; }

private:
    SkeletonPaletteCache paletteCache{};
    SkinnedAnimationBudget budget{};
    SkinnedPaletteResolver resolver{budget, paletteCache};
    std::uint32_t submitFrameIndex = 0;
};

}  // namespace Spark
