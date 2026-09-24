#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/** One transient camera shake impulse (Composite: multiple impulses sum each frame). */
struct CameraShakeImpulse {
    Vector2 amplitude{Vector2::Zero};
    float durationSeconds = 0.0F;
    float frequencyHz = 28.0F;
    float elapsedSeconds = 0.0F;
    std::uint32_t seed = 0;
};

/**
 * Damped camera shake driven by stacked impulses and optional trauma accumulation.
 * <c>Camera2DRigComponent</c> reads <c>GetOffset()</c> when writing the final camera pose.
 *
 * Runs at priority 295 (before <c>Camera2DRigComponent</c> at 300).
 */
class ScreenShakeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::ScreenShake;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 295; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Queue a directional shake burst. Amplitude is peak world-unit offset. */
    void AddImpulse(Vector2 amplitude, float durationSeconds, float frequencyHz = 28.0F) noexcept;

    /** Trauma-based shake (0..1); each call adds energy that decays over time. */
    void AddTrauma(float amount) noexcept;

    [[nodiscard]] const Vector2& GetOffset() const noexcept { return currentOffset; }
    [[nodiscard]] float GetTrauma() const noexcept { return trauma; }

    void Clear() noexcept;

    /** Advances simulation and returns the offset to apply this frame. */
    Vector2 Tick(float deltaSeconds) noexcept;

private:
    Array<CameraShakeImpulse> impulses{};
    Vector2 currentOffset{Vector2::Zero};
    float trauma = 0.0F;
    float traumaDecayPerSecond = 1.35F;
    float shakeTime = 0.0F;
    std::uint32_t nextSeed = 1;
};

}  // namespace Spark
