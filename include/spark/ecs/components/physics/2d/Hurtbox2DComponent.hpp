#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_shape)
    void SetShape(Hurtbox2DShape value) noexcept { shape = value; }
    SPARK_SCRIPT_BIND(get_shape)
    [[nodiscard]] Hurtbox2DShape GetShape() const noexcept { return shape; }

    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    SPARK_SCRIPT_BIND(get_local_offset)
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }

    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(float value) noexcept;
    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    SPARK_SCRIPT_BIND(set_half_extents)
    void SetHalfExtents(const Vector2& value) noexcept;
    SPARK_SCRIPT_BIND(get_half_extents)
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }

    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(std::uint16_t bits) noexcept;
    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }

    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(std::uint16_t bits) noexcept;
    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }

    SPARK_SCRIPT_BIND(set_invulnerability_seconds)
    void SetInvulnerabilitySeconds(const float seconds) noexcept;
    SPARK_SCRIPT_BIND(get_invulnerability_seconds)
    [[nodiscard]] float GetInvulnerabilitySeconds() const noexcept { return invulnerabilitySeconds; }

    SPARK_SCRIPT_BIND(is_invulnerable)
    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerabilityRemaining > 0.0F; }

    /** Called by <c>TryApplyCombatDamage2D</c> after successful damage. */
    SPARK_SCRIPT_BIND(notify_damage_received)
    void NotifyDamageReceived() noexcept;

    /** Pushes collider settings to sibling physics components. */
    SPARK_SCRIPT_BIND(sync_collider)
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
