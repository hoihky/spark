#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <algorithm>

namespace Spark {

class GameWorld;
struct FrameTiming;
struct CharacterController2DSettings;
class CharacterControllerWorld2D;

/**
 * Platformer motor layered on <c>Rigidbody2DComponent</c> + a 2D collider.
 * Call <c>PhysicsSubsystem::Simulate2D</c> (or <c>CharacterControllerWorld2D</c> prepare/finalize)
 * each frame after setting move intent and optional jump / drop-through requests.
 */
class CharacterController2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CharacterController2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    CharacterController2DComponent() = default;

    /** Horizontal desired speed (world units/s). Applied to <c>Rigidbody2D</c> velocity X each step. */
    [[nodiscard]] float GetMoveSpeed() const noexcept { return moveSpeed; }
    void SetMoveSpeed(const float speed) noexcept { moveSpeed = std::max(0.0F, speed); }

    [[nodiscard]] float GetJumpSpeed() const noexcept { return jumpSpeed; }
    void SetJumpSpeed(const float speed) noexcept { jumpSpeed = std::max(0.0F, speed); }

    [[nodiscard]] float GetCoyoteTimeSeconds() const noexcept { return coyoteTimeSeconds; }
    void SetCoyoteTimeSeconds(const float seconds) noexcept { coyoteTimeSeconds = std::max(0.0F, seconds); }

    [[nodiscard]] float GetJumpBufferSeconds() const noexcept { return jumpBufferSeconds; }
    void SetJumpBufferSeconds(const float seconds) noexcept { jumpBufferSeconds = std::max(0.0F, seconds); }

    /** Downward probe distance for ground snap after physics (world units). */
    [[nodiscard]] float GetSnapToGroundDistance() const noexcept { return snapToGroundDistance; }
    void SetSnapToGroundDistance(const float distance) noexcept {
        snapToGroundDistance = std::max(0.0F, distance);
    }

    /** Max walkable ground normal Y = cos(degrees). 90 = any upward-facing surface counts. */
    [[nodiscard]] float GetSlopeLimitDegrees() const noexcept { return slopeLimitDegrees; }
    void SetSlopeLimitDegrees(const float degrees) noexcept {
        slopeLimitDegrees = std::clamp(degrees, 0.0F, 89.0F);
    }

    [[nodiscard]] float GetSkinWidth() const noexcept { return skinWidth; }
    void SetSkinWidth(const float width) noexcept { skinWidth = std::max(0.0F, width); }

    /** Normalized horizontal input in [-1, 1]. Set each frame before simulation. */
    void SetMoveInputX(const float normalized) noexcept { moveInputX = std::clamp(normalized, -1.0F, 1.0F); }
    [[nodiscard]] float GetMoveInputX() const noexcept { return moveInputX; }

    /** Queues a jump for the next prepare step (honors coyote time + jump buffer). */
    void RequestJump() noexcept { jumpRequested = true; }

    /** When true, one-way platforms are ignored for this frame (drop-through). Cleared after prepare. */
    void SetDropThroughOneWay(const bool drop) noexcept { dropThroughOneWay = drop; }
    [[nodiscard]] bool GetDropThroughOneWay() const noexcept { return dropThroughOneWay; }

    [[nodiscard]] bool IsGrounded() const noexcept { return grounded; }
    [[nodiscard]] bool WasGroundedLastFrame() const noexcept { return wasGroundedLastFrame; }

private:
    friend void SimulateCharacterControllers2D(
            GameWorld& world,
            const FrameTiming& timing,
            const CharacterController2DSettings& settings);
    friend class CharacterControllerWorld2D;

    float moveSpeed = 11.0F;
    float jumpSpeed = 13.2F;
    float coyoteTimeSeconds = 0.12F;
    float jumpBufferSeconds = 0.1F;
    float snapToGroundDistance = 0.08F;
    float slopeLimitDegrees = 50.0F;
    float skinWidth = 0.02F;
    float moveInputX = 0.0F;
    float coyoteTimeRemaining = 0.0F;
    float jumpBufferRemaining = 0.0F;
    bool jumpRequested = false;
    bool dropThroughOneWay = false;
    bool grounded = false;
    bool wasGroundedLastFrame = false;
};

}  // namespace Spark
