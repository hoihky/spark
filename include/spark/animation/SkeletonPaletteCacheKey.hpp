#pragma once

#include "spark/animation/AnimLoopMode.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class Skeleton;

/**
 * Immutable playback signature for skeleton palette sharing.
 * Quantizes time so NPCs on the same clip can reuse one palette solve per frame.
 */
class SkeletonPaletteCacheKey {
public:
    enum class PlaybackMode : std::uint8_t {
        SingleClip = 0,
        LocomotionBlend = 1,
        Crossfade = 2,
    };

    void CaptureFromAnimator(const AnimatorComponent& animator, float quantizationHz) noexcept;

    [[nodiscard]] bool Matches(const SkeletonPaletteCacheKey& other) const noexcept;
    [[nodiscard]] std::uint64_t Digest() const noexcept;

    [[nodiscard]] const Skeleton* GetSkeleton() const noexcept { return skeleton; }
    [[nodiscard]] PlaybackMode GetPlaybackMode() const noexcept { return playbackMode; }

private:
    static std::uint32_t QuantizeSeconds(float timeSec, float quantizationHz) noexcept;
    static std::uint16_t QuantizeBlend01(float blend01) noexcept;

    const Skeleton* skeleton = nullptr;
    PlaybackMode playbackMode = PlaybackMode::SingleClip;
    AnimLoopMode loopMode = AnimLoopMode::Loop;

    std::uint32_t primaryClip = 0;
    std::uint32_t primaryTimeTick = 0;

    std::uint32_t secondaryClip = 0;
    std::uint32_t secondaryTimeTick = 0;

    std::uint32_t tertiaryClip = 0;
    std::uint32_t tertiaryTimeTick = 0;

    std::uint16_t blendPacked = 0;
    std::uint16_t crossfadePacked = 0;
};

}  // namespace Spark
