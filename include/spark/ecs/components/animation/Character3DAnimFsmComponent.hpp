#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class AnimatorComponent;
class GameObject;
class IEngineContext;

/**
 * Selects <c>AnimatorComponent</c> locomotion clips from horizontal speed (XZ plane) with optional
 * 1D speed blend tree (idle / walk / run). Supports combat overlays (hurt / attack / stagger / death)
 * triggered from gameplay or <c>AiAgentComponent</c> blackboard (see <c>kAiBlackboardIntCharacter3DCombatCommand</c>).
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
     * Resolves locomotion and combat clips from <c>skeleton</c> clip names and applies them on this FSM.
     * <c>walkClipFallback</c> is used when no walk-named clip exists.
     */
    void ConfigureLocomotionFromSkeleton(const Skeleton& skeleton, std::uint32_t walkClipFallback = 0) noexcept;

    /** Pass index >= clip count to disable that overlay. Non-looping clips work best for combat. */
    void SetCombatClips(
            std::uint32_t attackClipIndex,
            std::uint32_t hurtClipIndex,
            std::uint32_t staggerClipIndex = kInvalidAnimClipIndex,
            std::uint32_t deathClipIndex = kInvalidAnimClipIndex) noexcept;
    /** Pass index >= clip count to disable attack overlay. */
    void SetAttackClip(std::uint32_t attackClipIndex) noexcept;
    void SetHurtClip(std::uint32_t hurtClipIndex) noexcept;
    void SetStaggerClip(std::uint32_t staggerClipIndex) noexcept;
    void SetDeathClip(std::uint32_t deathClipIndex) noexcept;

    void SetWalkSpeedThreshold(float metersPerSecond) noexcept;
    void SetRunSpeedThreshold(float metersPerSecond) noexcept;
    void SetCrossfadeDuration(float seconds) noexcept;

    /** When true (default), walk↔run uses a 1D speed blend tree instead of discrete clip switches. */
    void SetLocomotionBlendEnabled(bool enabled) noexcept { locomotionBlendEnabled = enabled; }
    [[nodiscard]] bool IsLocomotionBlendEnabled() const noexcept { return locomotionBlendEnabled; }

    /** When not <c>SIZE_MAX</c>, reads/writes <c>AiAgentComponent::GetBlackboard()</c> int at this slot. */
    void SetCombatBlackboardIntSlot(std::size_t slotOrMax) noexcept { combatBbSlot = slotOrMax; }
    [[nodiscard]] std::size_t GetCombatBlackboardIntSlot() const noexcept { return combatBbSlot; }

    void RequestHurt() noexcept;
    void RequestAttack() noexcept;
    void RequestStagger() noexcept;
    void RequestDeath() noexcept;

    /** When false, locomotion clips are not applied (manual <c>AnimatorComponent</c> control or combat overlay only). */
    void SetLocomotionDrivingEnabled(bool enabled) noexcept { locomotionDrivingEnabled = enabled; }
    [[nodiscard]] bool IsLocomotionDrivingEnabled() const noexcept { return locomotionDrivingEnabled; }

    /** Pins a clip until <c>ClearManualClip</c> or locomotion is re-enabled via input in demos. */
    void SetManualClip(std::uint32_t clipIndex, AnimLoopMode loopMode) noexcept;
    void ClearManualClip() noexcept;
    [[nodiscard]] bool IsManualClipActive() const noexcept { return manualClipActive; }

    /**
     * Optional per-frame gait from gameplay input (e.g. WASD + Shift). When not set before <c>OnUpdate</c>,
     * locomotion falls back to horizontal speed thresholds.
     */
    void SetLocomotionInput(bool moving, bool sprint) noexcept;

    [[nodiscard]] std::uint32_t GetIdleClipIndex() const noexcept { return idleClip; }
    [[nodiscard]] std::uint32_t GetWalkClipIndex() const noexcept { return walkClip; }
    [[nodiscard]] std::uint32_t GetRunClipIndex() const noexcept { return runClip; }
    [[nodiscard]] std::uint32_t GetAttackClipIndex() const noexcept { return attackClip; }
    [[nodiscard]] std::uint32_t GetHurtClipIndex() const noexcept { return hurtClip; }
    [[nodiscard]] std::uint32_t GetStaggerClipIndex() const noexcept { return staggerClip; }
    [[nodiscard]] std::uint32_t GetDeathClipIndex() const noexcept { return deathClip; }
    [[nodiscard]] float GetWalkSpeedThreshold() const noexcept { return walkThresh; }
    [[nodiscard]] float GetRunSpeedThreshold() const noexcept { return runThresh; }
    [[nodiscard]] float GetCrossfadeDuration() const noexcept { return crossfadeDuration; }
    [[nodiscard]] std::uint32_t GetManualClipIndex() const noexcept { return manualClip; }
    [[nodiscard]] AnimLoopMode GetManualClipLoopMode() const noexcept { return manualLoop; }
    /** True after a death clip has finished playing (character stays in death pose). */
    [[nodiscard]] bool IsDead() const noexcept { return deathLocked; }

private:
    struct CombatSelection {
        std::uint32_t clip = 0;
        AnimLoopMode loopMode = AnimLoopMode::Loop;
        bool active = false;
        int playingCmd = 0;
    };

    [[nodiscard]] bool ClipValid_(const AnimatorComponent& anim, std::uint32_t clip) const noexcept;
    void ApplyClip_(AnimatorComponent& anim, std::uint32_t want, AnimLoopMode loopMode) noexcept;
    void ApplyLocomotionBlend_(AnimatorComponent& anim, float speedMetersPerSecond) noexcept;
    void ApplyLocomotionDiscrete_(AnimatorComponent& anim, bool moving, bool wantSprint) noexcept;
    [[nodiscard]] CombatSelection ResolveCombat_(
            AnimatorComponent& anim,
            GameObject& owner,
            int combatCmd) noexcept;
    void MaybeClearBlackboardCombat_(GameObject& owner, int playingCmd) noexcept;
    [[nodiscard]] float MeasureLocomotionSpeed_(const GameObject& owner, const FrameTiming& timing) noexcept;
    [[nodiscard]] float ResolveLocomotionSpeedParam_(
            const GameObject& owner,
            const FrameTiming& timing,
            bool moving,
            bool wantSprint) noexcept;

    std::uint32_t idleClip = 0;
    std::uint32_t walkClip = 1;
    std::uint32_t runClip = kInvalidAnimClipIndex;
    std::uint32_t attackClip = kInvalidAnimClipIndex;
    std::uint32_t hurtClip = kInvalidAnimClipIndex;
    std::uint32_t staggerClip = kInvalidAnimClipIndex;
    std::uint32_t deathClip = kInvalidAnimClipIndex;
    float walkThresh = 0.35F;
    float runThresh = 2.2F;
    float crossfadeDuration = 0.18F;

    std::uint8_t pendingCombat = 0;  // 1 hurt, 2 attack, 3 stagger, 4 death
    bool deathLocked = false;
    bool locomotionDrivingEnabled = true;
    bool locomotionBlendEnabled = true;
    bool manualClipActive = false;
    std::uint32_t manualClip = 0;
    AnimLoopMode manualLoop = AnimLoopMode::Loop;
    bool hasLocomotionInput = false;
    bool inputMoving = false;
    bool inputSprint = false;
    bool hasLastWorldPos = false;
    Vector3 lastWorldPos{Vector3::Zero};
    std::size_t combatBbSlot = static_cast<std::size_t>(-1);
};

}  // namespace Spark
