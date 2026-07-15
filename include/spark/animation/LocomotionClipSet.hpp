#pragma once

#include <cstdint>

namespace Spark {

class Skeleton;

/** Sentinel clip index — pass to <c>SetAttackClip</c> / locomotion run slot to disable that state. */
constexpr std::uint32_t kInvalidAnimClipIndex = 0xFFFFFFFFu;

/** Resolved idle / walk / run / attack clip indices from a <c>Skeleton</c> clip name table. */
struct LocomotionClipSet {
    std::uint32_t idle = 0;
    std::uint32_t walk = 0;
    std::uint32_t run = kInvalidAnimClipIndex;
    std::uint32_t attack = kInvalidAnimClipIndex;
};

/**
 * Finds locomotion clips by case-insensitive exact name, then substring (e.g. Survey, Walk, Run).
 * <c>walkFallback</c> is used when no walk-named clip exists (e.g. glTF loader heuristic index).
 */
[[nodiscard]] LocomotionClipSet ResolveLocomotionClipsFromSkeleton(
        const Skeleton& skeleton,
        std::uint32_t walkFallback = 0) noexcept;

}  // namespace Spark
