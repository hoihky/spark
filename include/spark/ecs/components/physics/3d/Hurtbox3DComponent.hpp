#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/**
 * Passive spherical hurt volume for 3D combat queries (no physics collider required).
 * Damage should flow through <c>Combat3DTarget::ApplyDamage</c>.
 */
class Hurtbox3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Hurtbox3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector3& offset) noexcept { localOffset = offset; }
    SPARK_SCRIPT_BIND(get_local_offset)
    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return localOffset; }

    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(float value) noexcept;
    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }
    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }

    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }
    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }

    SPARK_SCRIPT_BIND(set_invulnerability_seconds)
    void SetInvulnerabilitySeconds(const float seconds) noexcept;
    SPARK_SCRIPT_BIND(get_invulnerability_seconds)
    [[nodiscard]] float GetInvulnerabilitySeconds() const noexcept { return invulnerabilitySeconds; }

    SPARK_SCRIPT_BIND(is_invulnerable)
    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerabilityRemaining > 0.0F; }

    SPARK_SCRIPT_BIND(notify_damage_received)
    void NotifyDamageReceived() noexcept;

    [[nodiscard]] Vector3 GetWorldCenter(const GameObject& owner) const noexcept;
    [[nodiscard]] float GetWorldRadius(const GameObject& owner) const noexcept;

    [[nodiscard]] bool MatchesQueryFilter(const ColliderFilter& queryFilter) const noexcept;
    [[nodiscard]] bool OverlapsWorldSphere(
            const GameObject& owner,
            const Vector3& center,
            float radius) const noexcept;

private:
    Vector3 localOffset{0.0F, 0.9F, 0.0F};
    float radius = 0.55F;
    float invulnerabilitySeconds = 0.35F;
    float invulnerabilityRemaining = 0.0F;
    std::uint16_t categoryBits = 1u << 2;  // Enemy default
    std::uint16_t maskBits = 0xFFFFu;
    bool enabled = true;
};

}  // namespace Spark
