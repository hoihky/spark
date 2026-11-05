#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

enum class RigidbodyBodyType2D : std::uint8_t {
    /** Moved only by script; still participates as a movable blocker when paired with a collider (future). */
    Kinematic = 0,
    /** No simulation; use with BoxCollider2D / CircleCollider2D for static level geometry (optional). */
    Static = 1,
    /** Velocity integration + resolution against static 2D colliders (box/circle). */
    Dynamic = 2,
};

/**
 * 2D velocity-based body (XY). Grounding is updated by <c>PhysicsWorld2D</c> / <c>PhysicsSubsystem</c> when using a 2D collider + Transform.
 */
class Rigidbody2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Rigidbody2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit Rigidbody2DComponent(
            RigidbodyBodyType2D bodyType = RigidbodyBodyType2D::Dynamic,
            float gravityScaleIn = 1.0F) noexcept
            : bodyType(bodyType), gravityScale(gravityScaleIn) {}

    [[nodiscard]] RigidbodyBodyType2D GetBodyType() const noexcept { return bodyType; }
    void SetBodyType(RigidbodyBodyType2D t) noexcept { bodyType = t; }

    [[nodiscard]] float GetGravityScale() const noexcept { return gravityScale; }
    void SetGravityScale(float g) noexcept { gravityScale = g; }

    [[nodiscard]] const Vector2& GetVelocity() const noexcept { return velocity; }
    void SetVelocity(const Vector2& v) noexcept { velocity = v; }
    void SetVelocityX(float x) noexcept { velocity.x = x; }
    void SetVelocityY(float y) noexcept { velocity.y = y; }

    [[nodiscard]] bool IsGrounded() const noexcept { return grounded; }
    void SetGrounded(bool g) noexcept { grounded = g; }

private:
    RigidbodyBodyType2D bodyType = RigidbodyBodyType2D::Dynamic;
    float gravityScale = 1.0F;
    Vector2 velocity{Vector2::Zero};
    bool grounded = false;
};

}  // namespace Spark
