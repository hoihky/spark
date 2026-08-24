#pragma once

#include "spark/animation/Skeleton.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class SkinnedMeshComponent;

/**
 * Resolves a skin joint palette for scene submit (Strategy: animated clip vs bind/rest pose).
 */
class SkinnedMeshPalette {
public:
    [[nodiscard]] static bool TryFill(
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Matrix4* outPalette,
            std::uint32_t paletteMax) noexcept;
};

}  // namespace Spark
