#pragma once

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/** Payload delivered to managed animation-event callbacks (C ABI stable). */
struct AnimationEventScriptPayload {
    const char* eventName = nullptr;
    std::uint32_t clipIndex = 0;
    float timeSeconds = 0.0F;
};

using AnimationEventScriptCallback = void (*)(void* userData, GameObject* owner, const AnimationEventScriptPayload& payload);

/** One normalized-time marker on a clip (0..1 relative to clip duration). */
struct AnimationEventMarker {
    std::uint32_t clipIndex = 0;
    float normalizedTime = 0.0F;
    Utf8String eventName;
};

/**
 * Fires <c>SignalId::AnimationEvent</c> when a sibling <c>AnimatorComponent</c> crosses configured markers.
 * Runs immediately after animator playback (priority 210).
 */
class AnimationEventReceiverComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AnimationEventReceiver;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 210; }

    [[nodiscard]] const Array<AnimationEventMarker>& GetMarkers() const noexcept { return markers; }
    Array<AnimationEventMarker>& GetMarkers() noexcept { return markers; }

    void ClearMarkers() noexcept { markers.Clear(); firedMask.Clear(); }
    void AddMarker(std::uint32_t clipIndex, float normalizedTime, const char* eventName);
    /** Replaces markers with absolute-time events from <c>skeleton</c>, converted to normalized times. */
    void ImportFromSkeleton(const Skeleton& skeleton);

    /** Optional managed/script callback (invoked after sibling <c>OnSignal</c> dispatch). */
    void SetScriptCallback(AnimationEventScriptCallback callback, void* userData) noexcept;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    Array<AnimationEventMarker> markers{};
    /** Per-marker latch: bit i set after marker i fired for the current clip lap. */
    Array<std::uint8_t> firedMask{};
    AnimationEventScriptCallback scriptCallback = nullptr;
    void* scriptUserData = nullptr;
};

}  // namespace Spark
