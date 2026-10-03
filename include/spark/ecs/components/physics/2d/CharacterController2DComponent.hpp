#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <algorithm>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Platformer (gravity + jump) vs top-down (XY velocity, no ground snap). */
enum class CharacterController2DMotorMode : std::uint8_t {
    Platformer = 0,
    TopDown = 1,
};

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
    SPARK_SCRIPT_BIND(get_move_speed)
    [[nodiscard]] float GetMoveSpeed() const noexcept { return moveSpeed; }
    SPARK_SCRIPT_BIND(set_move_speed)
    void SetMoveSpeed(const float speed) noexcept { moveSpeed = std::max(0.0F, speed); }

    SPARK_SCRIPT_BIND(get_jump_speed)
    [[nodiscard]] float GetJumpSpeed() const noexcept { return jumpSpeed; }
    SPARK_SCRIPT_BIND(set_jump_speed)
    void SetJumpSpeed(const float speed) noexcept { jumpSpeed = std::max(0.0F, speed); }

    SPARK_SCRIPT_BIND(get_coyote_time_seconds)
    [[nodiscard]] float GetCoyoteTimeSeconds() const noexcept { return coyoteTimeSeconds; }
    SPARK_SCRIPT_BIND(set_coyote_time_seconds)
    void SetCoyoteTimeSeconds(const float seconds) noexcept { coyoteTimeSeconds = std::max(0.0F, seconds); }

    SPARK_SCRIPT_BIND(get_jump_buffer_seconds)
    [[nodiscard]] float GetJumpBufferSeconds() const noexcept { return jumpBufferSeconds; }
    SPARK_SCRIPT_BIND(set_jump_buffer_seconds)
    void SetJumpBufferSeconds(const float seconds) noexcept { jumpBufferSeconds = std::max(0.0F, seconds); }

    /** Downward probe distance for ground snap after physics (world units). */
    SPARK_SCRIPT_BIND(get_snap_to_ground_distance)
    [[nodiscard]] float GetSnapToGroundDistance() const noexcept { return snapToGroundDistance; }
    void SetSnapToGroundDistance(const float distance) noexcept {
        snapToGroundDistance = std::max(0.0F, distance);
    }

    /** Max walkable ground normal Y = cos(degrees). 90 = any upward-facing surface counts. */
    SPARK_SCRIPT_BIND(get_slope_limit_degrees)
    [[nodiscard]] float GetSlopeLimitDegrees() const noexcept { return slopeLimitDegrees; }
    void SetSlopeLimitDegrees(const float degrees) noexcept {
        slopeLimitDegrees = std::clamp(degrees, 0.0F, 89.0F);
    }

    SPARK_SCRIPT_BIND(get_skin_width)
    [[nodiscard]] float GetSkinWidth() const noexcept { return skinWidth; }
    SPARK_SCRIPT_BIND(set_skin_width)
    void SetSkinWidth(const float width) noexcept { skinWidth = std::max(0.0F, width); }

    /** Normalized horizontal input in [-1, 1]. Set each frame before simulation. */
    SPARK_SCRIPT_BIND(set_move_input_x)
    void SetMoveInputX(const float normalized) noexcept { moveInputX = std::clamp(normalized, -1.0F, 1.0F); }
    SPARK_SCRIPT_BIND(get_move_input_x)
    [[nodiscard]] float GetMoveInputX() const noexcept { return moveInputX; }

    SPARK_SCRIPT_BIND(set_move_input_y)
    void SetMoveInputY(const float normalized) noexcept { moveInputY = std::clamp(normalized, -1.0F, 1.0F); }
    SPARK_SCRIPT_BIND(get_move_input_y)
    [[nodiscard]] float GetMoveInputY() const noexcept { return moveInputY; }

    SPARK_SCRIPT_BIND(set_motor_mode)
    void SetMotorMode(const CharacterController2DMotorMode mode) noexcept { motorMode = mode; }
    SPARK_SCRIPT_BIND(get_motor_mode)
    [[nodiscard]] CharacterController2DMotorMode GetMotorMode() const noexcept { return motorMode; }

    /** Queues a jump for the next prepare step (honors coyote time + jump buffer). */
    SPARK_SCRIPT_BIND(request_jump)
    void RequestJump() noexcept { jumpRequested = true; }

    /** When true, one-way platforms are ignored for this frame (drop-through). Cleared after prepare. */
    SPARK_SCRIPT_BIND(set_drop_through_one_way)
    void SetDropThroughOneWay(const bool drop) noexcept { dropThroughOneWay = drop; }
    SPARK_SCRIPT_BIND(get_drop_through_one_way)
    [[nodiscard]] bool GetDropThroughOneWay() const noexcept { return dropThroughOneWay; }

    SPARK_SCRIPT_BIND(is_grounded)
    [[nodiscard]] bool IsGrounded() const noexcept { return grounded; }
    SPARK_SCRIPT_BIND(is_was_grounded_last_frame)
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
    float moveInputY = 0.0F;
    CharacterController2DMotorMode motorMode = CharacterController2DMotorMode::Platformer;
    float coyoteTimeRemaining = 0.0F;
    float jumpBufferRemaining = 0.0F;
    bool jumpRequested = false;
    bool dropThroughOneWay = false;
    bool grounded = false;
    bool wasGroundedLastFrame = false;
};

}  // namespace Spark
