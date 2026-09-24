#include "spark/ecs/components/animation/SpriteAnimationEventReceiverComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

namespace {

std::uint64_t FloatBits(const float value) noexcept {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return static_cast<std::uint64_t>(bits);
}

[[nodiscard]] float ComputeClipDurationSeconds(const SpriteAnimationClip& clip) noexcept {
    if (clip.frameCount == 0U) {
        return 0.0F;
    }
    const float fps = (clip.framesPerSecond > 1.0e-6F) ? clip.framesPerSecond : 8.0F;
    return static_cast<float>(clip.frameCount) / fps;
}

}  // namespace

void SpriteAnimationEventReceiverComponent::ClearMarkers() noexcept {
    markers.Clear();
    firedMask.Clear();
}

void SpriteAnimationEventReceiverComponent::AddMarker(
        const std::uint32_t clipIndex,
        const float normalizedTime,
        const char* eventName) {
    SpriteAnimationEventMarker marker{};
    marker.clipIndex = clipIndex;
    marker.normalizedTime = std::clamp(normalizedTime, 0.0F, 1.0F);
    marker.eventName = Utf8String(eventName != nullptr ? eventName : "");
    markers.PushBack(MoveTemp(marker));
    firedMask.PushBack(0);
}

void SpriteAnimationEventReceiverComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    const SpriteAnimatorComponent* animator = owner.GetComponent<SpriteAnimatorComponent>();
    if (animator == nullptr) {
        return;
    }
    const SpriteAnimationClip* clip = animator->GetCurrentClip();
    if (clip == nullptr) {
        return;
    }

    const std::uint32_t activeClip = animator->GetClipIndex();
    const float clipDuration = ComputeClipDurationSeconds(*clip);
    if (clipDuration <= 1.0e-5F) {
        return;
    }

    const float currentTime = animator->GetTimeInClipSeconds();
    const float previousTime = std::max(0.0F, currentTime - timing.deltaTimeSeconds);
    const bool looped = clip->loop && previousTime > currentTime;

    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (markers[i].clipIndex != activeClip) {
            firedMask[i] = 0;
            continue;
        }

        const float markerTime = markers[i].normalizedTime * clipDuration;
        const bool crossed = looped ? (currentTime >= markerTime || previousTime <= markerTime)
                                    : (previousTime < markerTime && currentTime >= markerTime);
        if (!crossed || firedMask[i] != 0) {
            if (looped && currentTime < markerTime) {
                firedMask[i] = 0;
            }
            continue;
        }

        firedMask[i] = 1;
        SignalPayload payload{};
        payload.ptr = markers[i].eventName.CStr();
        payload.a = activeClip;
        payload.b = FloatBits(markerTime);
        owner.EmitSignal(SignalId::SpriteAnimationEvent, payload, this);
    }
}

}  // namespace Spark
