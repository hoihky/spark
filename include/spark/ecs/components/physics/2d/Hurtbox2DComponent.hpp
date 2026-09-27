#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/** Passive hurt volume shape baked into a trigger collider for query-based weapons. */
enum class Hurtbox2DShape : std::uint8_t {
    Circle = 0,
    Box = 1,
};

/**
 * Configures a sibling trigger collider as a damage receiver (layer mask + optional i-frames).
 * Attack systems (<c>AnimationHitbox2DComponent</c>, <c>Projectile2DComponent</c>) should apply damage
 * via <c>TryApplyCombatDamage2D</c> so invulnerability is honored consistently.
 */
class Hurtbox2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Hurtbox2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 40; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetShape(Hurtbox2DShape value) noexcept { shape = value; }
    [[nodiscard]] Hurtbox2DShape GetShape() const noexcept { return shape; }

    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }

    void SetRadius(float value) noexcept;
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    void SetHalfExtents(const Vector2& value) noexcept;
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }

    void SetCategoryBits(std::uint16_t bits) noexcept;
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }

    void SetMaskBits(std::uint16_t bits) noexcept;
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }

    void SetInvulnerabilitySeconds(const float seconds) noexcept;
    [[nodiscard]] float GetInvulnerabilitySeconds() const noexcept { return invulnerabilitySeconds; }

    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerabilityRemaining > 0.0F; }

    /** Called by <c>TryApplyCombatDamage2D</c> after successful damage. */
    void NotifyDamageReceived() noexcept;

    /** Pushes collider settings to sibling physics components. */
    void SyncCollider(GameObject& owner) noexcept;

private:
    Hurtbox2DShape shape = Hurtbox2DShape::Circle;
    Vector2 localOffset{Vector2::Zero};
    Vector2 halfExtents{0.5F, 0.5F};
    float radius = 0.5F;
    float invulnerabilitySeconds = 0.0F;
    float invulnerabilityRemaining = 0.0F;
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool enabled = true;
};

}  // namespace Spark
