#include "spark/animation/AnimationClipEvent.hpp"
#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

std::uint64_t FloatBits(float v) noexcept {
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(v));
    std::memcpy(&bits, &v, sizeof(bits));
    return static_cast<std::uint64_t>(bits);
}

bool ClipIsActive(const std::uint32_t clip, const std::uint32_t* activeClips, const std::size_t activeCount) noexcept {
    for (std::size_t i = 0; i < activeCount; ++i) {
        if (activeClips[i] == clip) {
            return true;
        }
    }
    return false;
}

}  // namespace

void AnimationEventReceiverComponent::AddMarker(
        const std::uint32_t clipIndex,
        const float normalizedTime,
        const char* eventName) {
    AnimationEventMarker m{};
    m.clipIndex = clipIndex;
    m.normalizedTime = std::clamp(normalizedTime, 0.0F, 1.0F);
    m.eventName = Utf8String(eventName != nullptr ? eventName : "");
    markers.PushBack(MoveTemp(m));
    firedMask.PushBack(0);
}

void AnimationEventReceiverComponent::ImportFromSkeleton(const Skeleton& skeleton) {
    ClearMarkers();
    const std::uint32_t clipCount = skeleton.GetClipCount();
    for (std::uint32_t ci = 0; ci < clipCount; ++ci) {
        const float dur = skeleton.GetClipDuration(ci);
        if (dur <= 1.0e-5F) {
            continue;
        }
        const Array<AnimationClipEvent>& events = skeleton.GetClipEvents(ci);
        for (std::size_t ei = 0; ei < events.GetSize(); ++ei) {
            const float normalized = std::clamp(events[ei].timeSeconds / dur, 0.0F, 1.0F);
            AddMarker(ci, normalized, events[ei].name.CStr());
        }
    }
}

void AnimationEventReceiverComponent::SetScriptCallback(
        const AnimationEventScriptCallback callback,
        void* const userData) noexcept {
    scriptCallback = callback;
    scriptUserData = userData;
}

void AnimationEventReceiverComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    const AnimatorComponent* animator = owner.GetComponent<AnimatorComponent>();
    if (animator == nullptr || !animator->GetSkeleton()) {
        return;
    }

    std::uint32_t activeClips[2]{};
    std::size_t activeClipCount = 0;
    if (animator->IsLocomotionBlending()) {
        activeClips[0] = animator->GetLocomotionBlendClipA();
        activeClips[1] = animator->GetLocomotionBlendClipB();
        activeClipCount = 2;
    } else {
        activeClips[0] = animator->GetClipIndex();
        activeClipCount = 1;
    }

    const float t = animator->GetTimeSeconds();
    const float prev = t - timing.deltaTimeSeconds * animator->GetSpeed();
    const bool looped = prev > t;

    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (!ClipIsActive(markers[i].clipIndex, activeClips, activeClipCount)) {
            firedMask[i] = 0;
            continue;
        }
        const float dur = animator->GetSkeleton()->GetClipDuration(markers[i].clipIndex);
        if (dur <= 1.0e-5F) {
            continue;
        }
        const float markerTime = markers[i].normalizedTime * dur;
        const bool crossed = looped ? (t >= markerTime || prev <= markerTime)
                                    : (prev < markerTime && t >= markerTime);
        if (!crossed || firedMask[i] != 0) {
            if (looped && t < markerTime) {
                firedMask[i] = 0;
            }
            continue;
        }
        firedMask[i] = 1;
        SignalPayload payload{};
        payload.ptr = markers[i].eventName.CStr();
        payload.a = markers[i].clipIndex;
        payload.b = FloatBits(markerTime);
        owner.EmitSignal(SignalId::AnimationEvent, payload, this);
        if (scriptCallback != nullptr) {
            AnimationEventScriptPayload scriptPayload{};
            scriptPayload.eventName = markers[i].eventName.CStr();
            scriptPayload.clipIndex = markers[i].clipIndex;
            scriptPayload.timeSeconds = markerTime;
            scriptCallback(scriptUserData, &owner, scriptPayload);
        }
    }
}

}  // namespace Spark
