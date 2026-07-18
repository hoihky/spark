#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

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

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    Array<AnimationEventMarker> markers{};
    /** Per-marker latch: bit i set after marker i fired for the current clip lap. */
    Array<std::uint8_t> firedMask{};
};

}  // namespace Spark
