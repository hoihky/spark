#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <algorithm>

namespace Spark {

class GameWorld;
struct FrameTiming;
struct CharacterController3DSettings;
class CharacterControllerWorld3D;

/**
 * Kinematic character motor (sphere vs static box/capsule colliders). Does not use <c>Rigidbody3DComponent</c>.
 * Each frame: set world-space move intent with <c>SetMoveInput</c>, then call
 * <c>SimulateCharacterControllers3D</c> from gameplay (typically before or instead of dynamic sphere physics).
 *
 * Transform translation is the **feet anchor** when <c>centerOffset</c> is <c>(0, radius, 0)</c> (default).
 */
class CharacterController3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CharacterController3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit CharacterController3DComponent(
            float radiusIn = 0.4F,
            Vector3 centerOffsetIn = Vector3{0.0F, 0.4F, 0.0F}) noexcept
            : radius(radiusIn), centerOffset(centerOffsetIn) {}

    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    void SetRadius(const float r) noexcept { radius = std::max(0.05F, r); }

    /** Sphere center in object local space (feet-at-origin convention: Y = radius). */
    [[nodiscard]] const Vector3& GetCenterOffset() const noexcept { return centerOffset; }
    void SetCenterOffset(const Vector3& o) noexcept { centerOffset = o; }

    /** Max walkable slope in degrees (0 = vertical wall only, 90 = any surface counts as ground). */
    [[nodiscard]] float GetSlopeLimitDegrees() const noexcept { return slopeLimitDegrees; }
    void SetSlopeLimitDegrees(const float degrees) noexcept {
        slopeLimitDegrees = std::clamp(degrees, 0.0F, 89.0F);
    }

    /** Auto-step height for small static ledges (meters). */
    [[nodiscard]] float GetStepOffset() const noexcept { return stepOffset; }
    void SetStepOffset(const float meters) noexcept { stepOffset = std::max(0.0F, meters); }

    [[nodiscard]] float GetSkinWidth() const noexcept { return skinWidth; }
    void SetSkinWidth(const float w) noexcept { skinWidth = std::max(0.0F, w); }

    [[nodiscard]] float GetGravityScale() const noexcept { return gravityScale; }
    void SetGravityScale(const float g) noexcept { gravityScale = g; }

    /**
     * World-space desired horizontal velocity (m/s). Y is ignored here; vertical motion comes from gravity /
     * grounding. Call each frame before <c>SimulateCharacterControllers3D</c>.
     */
    void SetMoveInput(const Vector3& worldVelocity) noexcept {
        moveInput.x = worldVelocity.x;
        moveInput.z = worldVelocity.z;
    }

    [[nodiscard]] const Vector3& GetMoveInput() const noexcept { return moveInput; }

    /** Integrated velocity after the last <c>SimulateCharacterControllers3D</c> step. */
    [[nodiscard]] const Vector3& GetVelocity() const noexcept { return velocity; }

    [[nodiscard]] bool IsGrounded() const noexcept { return grounded; }

private:
    friend void SimulateCharacterControllers3D(
            GameWorld& world,
            const FrameTiming& timing,
            const CharacterController3DSettings& settings);
    friend class CharacterControllerWorld3D;

    float radius = 0.4F;
    Vector3 centerOffset{0.0F, 0.4F, 0.0F};
    float slopeLimitDegrees = 50.0F;
    float stepOffset = 0.35F;
    float skinWidth = 0.02F;
    float gravityScale = 1.0F;
    Vector3 moveInput{Vector3::Zero};
    Vector3 velocity{Vector3::Zero};
    bool grounded = false;
};

}  // namespace Spark
