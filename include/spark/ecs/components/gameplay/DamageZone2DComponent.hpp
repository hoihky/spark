#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

#include <algorithm>
#include <cstdint>

namespace Spark {

class GameObject;

/** Volume shape used by <c>DamageZone2DComponent</c>. */
enum class DamageZone2DShape : std::uint8_t {
    Circle = 0,
    Box = 1,
};

/**
 * Area that applies continuous damage to overlapping hurtboxes / health each frame.
 * Uses physics overlap queries (same pipeline as projectiles) and <c>TryApplyCombatDamage2D</c>.
 */
class DamageZone2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::DamageZone2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 215; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetShape(DamageZone2DShape value) noexcept { shape = value; }
    [[nodiscard]] DamageZone2DShape GetShape() const noexcept { return shape; }

    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }

    void SetRadius(float value) noexcept;
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    void SetHalfExtents(const Vector2& value) noexcept;
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }

    void SetDamagePerSecond(const float value) noexcept { damagePerSecond = std::max(0.0F, value); }
    [[nodiscard]] float GetDamagePerSecond() const noexcept { return damagePerSecond; }

    void SetTargetFilter(const PhysicsQueryFilter2D& filter) noexcept { targetFilter = filter; }
    [[nodiscard]] const PhysicsQueryFilter2D& GetTargetFilter() const noexcept { return targetFilter; }

    void SetCategoryBits(std::uint16_t bits) noexcept;
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }

    void SetMaskBits(std::uint16_t bits) noexcept;
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }

    /** Optional instigator passed to damage routing (defaults to zone owner). */
    void SetInstigator(GameObject* object) noexcept { instigator = object; }
    [[nodiscard]] GameObject* GetInstigator() const noexcept { return instigator; }

    void SyncCollider(GameObject& owner) noexcept;

private:
    void ApplyDamageInVolume(GameObject& owner, float deltaSeconds) noexcept;

    DamageZone2DShape shape = DamageZone2DShape::Box;
    Vector2 localOffset{Vector2::Zero};
    Vector2 halfExtents{1.0F, 1.0F};
    float radius = 1.0F;
    float damagePerSecond = 10.0F;
    PhysicsQueryFilter2D targetFilter{};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    GameObject* instigator = nullptr;
    bool enabled = true;
};

}  // namespace Spark
