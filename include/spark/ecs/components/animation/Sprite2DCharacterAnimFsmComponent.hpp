#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class GameObject;
class IEngineContext;
class SpriteAnimatorComponent;

/**
 * How locomotion idle vs move is chosen from <c>Rigidbody2DComponent</c> velocity (when present).
 */
enum class Sprite2DAnimLocomotionSource : std::uint8_t {
    /** <c>std::fabs(v.x) > threshold</c> (side-scrollers). */
    HorizontalAbsVelX = 0,
    /** <c>v.x * v.x + v.y * v.y > threshold^2</c> (top-down). */
    SpeedSq = 1,
};

/**
 * Lightweight ARPG-style driver: selects <c>SpriteAnimatorComponent</c> clip from locomotion (2D velocity) and
 * optional combat overlays (hurt / attack). Combat may be triggered by one-shots from gameplay code or by
 * <c>AiAgentComponent</c> blackboard int (see <c>kAiBlackboardIntSprite2DCombatCommand</c>).
 *
 * Add **before** <c>SpriteAnimatorComponent</c> on the same <c>GameObject</c> so <c>OnUpdate</c> runs first each frame.
 */
class Sprite2DCharacterAnimFsmComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Sprite2DCharacterAnimFsm;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override {
        return ComponentUpdatePriority::AnimationDriver;
    }

    Sprite2DCharacterAnimFsmComponent() noexcept = default;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetLocomotionClips(std::uint32_t idleClipIndex, std::uint32_t moveClipIndex) noexcept;
    /**
     * Pass indices >= clip count to disable that overlay. Non-looping clips work best so the driver can clear
     * blackboard commands when playback finishes.
     */
    void SetCombatClips(std::uint32_t attackClipIndex, std::uint32_t hurtClipIndex) noexcept;

    void SetMoveSpeedThreshold(float worldUnits) noexcept;
    void SetLocomotionSource(Sprite2DAnimLocomotionSource s) noexcept { locomotionSource = s; }

    /** When not <c>SIZE_MAX</c>, reads/writes <c>AiAgentComponent::GetBlackboard()</c> int at this slot. */
    void SetCombatBlackboardIntSlot(std::size_t slotOrMax) noexcept { combatBbSlot = slotOrMax; }

    void RequestHurt() noexcept;
    void RequestAttack() noexcept;

private:
    [[nodiscard]] bool ClipValid_(const SpriteAnimatorComponent& anim, std::uint32_t clip) const noexcept;
    void MaybeClearBlackboardCombat_(GameObject& owner, int playingCmd) noexcept;

    std::uint32_t idleClip = 0;
    std::uint32_t moveClip = 1;
    std::uint32_t attackClip = 0xFFFFFFFFu;
    std::uint32_t hurtClip = 0xFFFFFFFFu;
    float moveThresh = 0.12F;
    Sprite2DAnimLocomotionSource locomotionSource = Sprite2DAnimLocomotionSource::HorizontalAbsVelX;
    std::size_t combatBbSlot = static_cast<std::size_t>(-1);

    std::uint8_t pendingCombat = 0;  // 1 hurt, 2 attack (same encoding as blackboard)
};

}  // namespace Spark
