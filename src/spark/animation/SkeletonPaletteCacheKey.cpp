#include "spark/animation/SkeletonPaletteCacheKey.hpp"

#include "spark/ecs/components/animation/AnimatorComponent.hpp"

#include <cmath>

namespace Spark {

namespace {

std::uint64_t MixHash(std::uint64_t seed, std::uint64_t value) noexcept {
    seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2);
    return seed;
}

}  // namespace

std::uint32_t SkeletonPaletteCacheKey::QuantizeSeconds(const float timeSec, const float quantizationHz) noexcept {
    if (quantizationHz <= 1.0e-4F) {
        return 0;
    }
    const float ticks = timeSec * quantizationHz;
    return static_cast<std::uint32_t>(std::floor(ticks + 0.5F));
}

std::uint16_t SkeletonPaletteCacheKey::QuantizeBlend01(const float blend01) noexcept {
    const float clamped = (blend01 < 0.0F) ? 0.0F : (blend01 > 1.0F ? 1.0F : blend01);
    return static_cast<std::uint16_t>(clamped * 65535.0F);
}

void SkeletonPaletteCacheKey::CaptureFromAnimator(
        const AnimatorComponent& animator,
        const float quantizationHz) noexcept {
    skeleton = animator.GetSkeleton().Get();
    loopMode = animator.GetLoopMode();
    primaryClip = animator.GetClipIndex();
    primaryTimeTick = QuantizeSeconds(animator.GetTimeSeconds(), quantizationHz);
    secondaryClip = 0;
    secondaryTimeTick = 0;
    tertiaryClip = 0;
    tertiaryTimeTick = 0;
    blendPacked = 0;
    crossfadePacked = 0;

    if (skeleton == nullptr) {
        playbackMode = PlaybackMode::SingleClip;
        return;
    }

    if (animator.crossfade.active) {
        playbackMode = PlaybackMode::Crossfade;
        crossfadePacked = QuantizeBlend01(animator.GetCrossfadeBlend01());
        primaryClip = animator.clipIndex;
        primaryTimeTick = QuantizeSeconds(animator.timeSeconds, quantizationHz);
        if (animator.crossfade.fromLocomotionBlend) {
            secondaryClip = animator.crossfade.fromBlendClipA;
            tertiaryClip = animator.crossfade.fromBlendClipB;
            tertiaryTimeTick = QuantizeSeconds(animator.crossfade.fromTime, quantizationHz);
            blendPacked = QuantizeBlend01(animator.crossfade.fromBlend01);
        } else {
            secondaryClip = animator.crossfade.fromClip;
            secondaryTimeTick = QuantizeSeconds(animator.crossfade.fromTime, quantizationHz);
        }
        return;
    }

    if (animator.locomotionBlend.active) {
        playbackMode = PlaybackMode::LocomotionBlend;
        primaryClip = animator.locomotionBlend.clipA;
        secondaryClip = animator.locomotionBlend.clipB;
        secondaryTimeTick = primaryTimeTick;
        blendPacked = QuantizeBlend01(animator.locomotionBlend.blend01);
        return;
    }

    playbackMode = PlaybackMode::SingleClip;
}

bool SkeletonPaletteCacheKey::Matches(const SkeletonPaletteCacheKey& other) const noexcept {
    if (skeleton != other.skeleton || playbackMode != other.playbackMode || loopMode != other.loopMode) {
        return false;
    }
    if (primaryClip != other.primaryClip || primaryTimeTick != other.primaryTimeTick) {
        return false;
    }
    if (secondaryClip != other.secondaryClip || secondaryTimeTick != other.secondaryTimeTick) {
        return false;
    }
    if (tertiaryClip != other.tertiaryClip || tertiaryTimeTick != other.tertiaryTimeTick) {
        return false;
    }
    return blendPacked == other.blendPacked && crossfadePacked == other.crossfadePacked;
}

std::uint64_t SkeletonPaletteCacheKey::Digest() const noexcept {
    std::uint64_t hash = MixHash(0, reinterpret_cast<std::uintptr_t>(skeleton));
    hash = MixHash(hash, static_cast<std::uint64_t>(playbackMode));
    hash = MixHash(hash, static_cast<std::uint64_t>(loopMode));
    hash = MixHash(hash, static_cast<std::uint64_t>(primaryClip));
    hash = MixHash(hash, static_cast<std::uint64_t>(primaryTimeTick));
    hash = MixHash(hash, static_cast<std::uint64_t>(secondaryClip));
    hash = MixHash(hash, static_cast<std::uint64_t>(secondaryTimeTick));
    hash = MixHash(hash, static_cast<std::uint64_t>(tertiaryClip));
    hash = MixHash(hash, static_cast<std::uint64_t>(tertiaryTimeTick));
    hash = MixHash(hash, static_cast<std::uint64_t>(blendPacked));
    hash = MixHash(hash, static_cast<std::uint64_t>(crossfadePacked));
    return hash;
}

}  // namespace Spark
