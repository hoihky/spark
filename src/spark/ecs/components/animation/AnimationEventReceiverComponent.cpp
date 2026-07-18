#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"

#include "spark/core/Utility.hpp"
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

void AnimationEventReceiverComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    const AnimatorComponent* animator = owner.GetComponent<AnimatorComponent>();
    if (animator == nullptr || !animator->GetSkeleton()) {
        return;
    }
    const std::uint32_t clip = animator->GetClipIndex();
    const float dur = animator->GetSkeleton()->GetClipDuration(clip);
    if (dur <= 1.0e-5F) {
        return;
    }
    const float t = animator->GetTimeSeconds();
    const float prev = t - timing.deltaTimeSeconds * animator->GetSpeed();
    const bool looped = prev > t;
    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (markers[i].clipIndex != clip) {
            firedMask[i] = 0;
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
        payload.a = clip;
        payload.b = FloatBits(markerTime);
        owner.EmitSignal(SignalId::AnimationEvent, payload, this);
    }
}

}  // namespace Spark
