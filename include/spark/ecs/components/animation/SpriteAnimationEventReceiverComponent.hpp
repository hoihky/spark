#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

/** Normalized-time marker on a <c>SpriteAnimatorComponent</c> clip (0..1 of clip duration). */
struct SpriteAnimationEventMarker {
    std::uint32_t clipIndex = 0;
    float normalizedTime = 0.0F;
    Utf8String eventName;
};

/**
 * Fires <c>SignalId::SpriteAnimationEvent</c> when a sibling <c>SpriteAnimatorComponent</c>
 * crosses configured markers (Observer over sprite playback).
 */
class SpriteAnimationEventReceiverComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpriteAnimationEventReceiver;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 215; }

    [[nodiscard]] const Array<SpriteAnimationEventMarker>& GetMarkers() const noexcept { return markers; }
    Array<SpriteAnimationEventMarker>& GetMarkers() noexcept { return markers; }

    void ClearMarkers() noexcept;
    void AddMarker(std::uint32_t clipIndex, float normalizedTime, const char* eventName);

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    Array<SpriteAnimationEventMarker> markers{};
    Array<std::uint8_t> firedMask{};
};

}  // namespace Spark
