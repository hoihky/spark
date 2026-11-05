#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/core/ColliderFilter.hpp"

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

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetLocalOffset(const Vector3& offset) noexcept { localOffset = offset; }
    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return localOffset; }

    void SetRadius(float value) noexcept;
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }

    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }

    void SetInvulnerabilitySeconds(const float seconds) noexcept;
    [[nodiscard]] float GetInvulnerabilitySeconds() const noexcept { return invulnerabilitySeconds; }

    [[nodiscard]] bool IsInvulnerable() const noexcept { return invulnerabilityRemaining > 0.0F; }

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
