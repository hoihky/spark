#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class AnimatorComponent;
class GameObject;
class IEngineContext;

/**
 * Selects <c>AnimatorComponent</c> locomotion clips from horizontal speed (XZ plane).
 * Optional one-shot attack clip uses <c>AnimLoopMode::Once</c>. Locomotion changes crossfade.
 *
 * Add **before** <c>AnimatorComponent</c> on the same <c>GameObject</c> so <c>OnUpdate</c> runs first.
 */
class Character3DAnimFsmComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Character3DAnimFsm;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override {
        return ComponentUpdatePriority::AnimationDriver;
    }

    Character3DAnimFsmComponent() noexcept = default;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetLocomotionClips(std::uint32_t idleClipIndex, std::uint32_t walkClipIndex, std::uint32_t runClipIndex) noexcept;
    /**
     * Resolves idle/walk/run/attack clips from <c>skeleton</c> clip names and applies them on this FSM.
     * <c>walkClipFallback</c> is used when no walk-named clip exists.
     */
    void ConfigureLocomotionFromSkeleton(const Skeleton& skeleton, std::uint32_t walkClipFallback = 0) noexcept;
    /** Pass index >= clip count to disable attack overlay. */
    void SetAttackClip(std::uint32_t attackClipIndex) noexcept;

    void SetWalkSpeedThreshold(float metersPerSecond) noexcept;
    void SetRunSpeedThreshold(float metersPerSecond) noexcept;
    void SetCrossfadeDuration(float seconds) noexcept;

    void RequestAttack() noexcept;

    /** When false, locomotion clips are not applied (manual <c>AnimatorComponent</c> control or attack overlay only). */
    void SetLocomotionDrivingEnabled(bool enabled) noexcept { locomotionDrivingEnabled = enabled; }
    [[nodiscard]] bool IsLocomotionDrivingEnabled() const noexcept { return locomotionDrivingEnabled; }

    /** Pins a clip until <c>ClearManualClip</c> or locomotion is re-enabled via WASD in demos. */
    void SetManualClip(std::uint32_t clipIndex, AnimLoopMode loopMode) noexcept;
    void ClearManualClip() noexcept;
    [[nodiscard]] bool IsManualClipActive() const noexcept { return manualClipActive; }

    /**
     * Optional per-frame gait from gameplay input (e.g. WASD + Shift). When not set before <c>OnUpdate</c>,
     * locomotion falls back to horizontal speed thresholds.
     */
    void SetLocomotionInput(bool moving, bool sprint) noexcept;

private:
    [[nodiscard]] bool ClipValid_(const AnimatorComponent& anim, std::uint32_t clip) const noexcept;
    void ApplyClip_(AnimatorComponent& anim, std::uint32_t want, AnimLoopMode loopMode) noexcept;
    [[nodiscard]] float MeasureLocomotionSpeed_(const GameObject& owner, const FrameTiming& timing) noexcept;

    std::uint32_t idleClip = 0;
    std::uint32_t walkClip = 1;
    std::uint32_t runClip = 0xFFFFFFFFu;
    std::uint32_t attackClip = 0xFFFFFFFFu;
    float walkThresh = 0.35F;
    float runThresh = 2.2F;
    float crossfadeDuration = 0.18F;

    bool pendingAttack = false;
    bool locomotionDrivingEnabled = true;
    bool manualClipActive = false;
    std::uint32_t manualClip = 0;
    AnimLoopMode manualLoop = AnimLoopMode::Loop;
    bool hasLocomotionInput = false;
    bool inputMoving = false;
    bool inputSprint = false;
    bool hasLastWorldPos = false;
    Vector3 lastWorldPos{Vector3::Zero};
};

}  // namespace Spark
