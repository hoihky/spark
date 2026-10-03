#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Function.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

#include <algorithm>
#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class GameWorld;

enum class Projectile2DShape : std::uint8_t {
    Circle = 0,
    Box = 1,
};

/** Invoked when a projectile damages a target (applied damage, instigator, victim). */
using Projectile2DHitCallback = Function<void(GameObject& projectile, GameObject& victim, float appliedDamage)>;

/**
 * Kinematic projectile: moves each frame, overlap-queries targets, applies damage through
 * <c>TryApplyCombatDamage2D</c>. Optional solid collision deactivates the shot.
 *
 * Deactivate with <c>Deactivate()</c> for pools; set <c>destroyOwnerOnDeactivate</c> for one-shot spawns.
 */
class Projectile2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Projectile2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 220; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Destroys owners queued during the last update step (after physics / combat tick). */
    static void ProcessDeferredDestroys(GameWorld& world) noexcept;

    SPARK_SCRIPT_BIND(set_active)
    void SetActive(const bool value) noexcept { active = value; }
    SPARK_SCRIPT_BIND(is_active)
    [[nodiscard]] bool IsActive() const noexcept { return active; }

    void Activate(
            const Vector2& worldPosition,
            const Vector2& velocity,
            GameObject* instigatorIn) noexcept;

    SPARK_SCRIPT_BIND(deactivate)
    void Deactivate() noexcept;

    SPARK_SCRIPT_BIND(set_shape)
    void SetShape(Projectile2DShape value) noexcept { shape = value; }
    SPARK_SCRIPT_BIND(get_shape)
    [[nodiscard]] Projectile2DShape GetShape() const noexcept { return shape; }

    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(const float value) noexcept;
    SPARK_SCRIPT_BIND(set_half_extents)
    void SetHalfExtents(const Vector2& value) noexcept;

    SPARK_SCRIPT_BIND(set_damage)
    void SetDamage(const float value) noexcept { damage = std::max(0.0F, value); }
    SPARK_SCRIPT_BIND(get_damage)
    [[nodiscard]] float GetDamage() const noexcept { return damage; }

    SPARK_SCRIPT_BIND(set_lifetime_seconds)
    void SetLifetimeSeconds(const float seconds) noexcept { lifetimeSeconds = std::max(0.0F, seconds); }
    SPARK_SCRIPT_BIND(get_lifetime_seconds)
    [[nodiscard]] float GetLifetimeSeconds() const noexcept { return lifetimeSeconds; }

    SPARK_SCRIPT_BIND(set_instigator)
    void SetInstigator(GameObject* object) noexcept { instigator = object; }
    SPARK_SCRIPT_BIND(get_instigator)
    [[nodiscard]] GameObject* GetInstigator() const noexcept { return instigator; }

    SPARK_SCRIPT_BIND(set_target_filter)
    void SetTargetFilter(const PhysicsQueryFilter2D& filter) noexcept { targetFilter = filter; }
    SPARK_SCRIPT_BIND(get_target_filter)
    [[nodiscard]] const PhysicsQueryFilter2D& GetTargetFilter() const noexcept { return targetFilter; }

    SPARK_SCRIPT_BIND(set_block_on_solid_hit)
    void SetBlockOnSolidHit(const bool value) noexcept { blockOnSolidHit = value; }
    SPARK_SCRIPT_BIND(get_block_on_solid_hit)
    [[nodiscard]] bool GetBlockOnSolidHit() const noexcept { return blockOnSolidHit; }

    SPARK_SCRIPT_BIND(set_destroy_owner_on_deactivate)
    void SetDestroyOwnerOnDeactivate(const bool value) noexcept { destroyOwnerOnDeactivate = value; }
    SPARK_SCRIPT_BIND(get_destroy_owner_on_deactivate)
    [[nodiscard]] bool GetDestroyOwnerOnDeactivate() const noexcept { return destroyOwnerOnDeactivate; }

    SPARK_SCRIPT_BIND(set_deactivate_on_first_hit)
    void SetDeactivateOnFirstHit(const bool value) noexcept { deactivateOnFirstHit = value; }
    SPARK_SCRIPT_BIND(get_deactivate_on_first_hit)
    [[nodiscard]] bool GetDeactivateOnFirstHit() const noexcept { return deactivateOnFirstHit; }

    void SetOnHit(Projectile2DHitCallback callback) { onHit = MoveTemp(callback); }

    SPARK_SCRIPT_BIND(get_velocity)
    [[nodiscard]] const Vector2& GetVelocity() const noexcept { return velocity; }
    SPARK_SCRIPT_BIND(set_velocity)
    void SetVelocity(const Vector2& value) noexcept { velocity = value; }

private:
    void TickLifetime(const float deltaSeconds) noexcept;
    void MoveOwner(GameObject& owner, const float deltaSeconds) noexcept;
    void ResolveWorldPose(const GameObject& owner, float& centerX, float& centerY) const noexcept;
    void QueryAndApplyHits(GameObject& owner) noexcept;
    bool QuerySolidBlock(GameWorld& world, const float centerX, const float centerY) const noexcept;
    void MarkDeferredDestroy(GameObject& owner) noexcept;

    Projectile2DShape shape = Projectile2DShape::Circle;
    Vector2 velocity{Vector2::Zero};
    Vector2 halfExtents{0.05F, 0.03F};
    float radius = 0.05F;
    float damage = 1.0F;
    float lifetimeSeconds = 3.0F;
    float ageSeconds = 0.0F;
    PhysicsQueryFilter2D targetFilter{};
    GameObject* instigator = nullptr;
    Array<GameObject*> hitVictims{};
    Projectile2DHitCallback onHit{};
    bool active = false;
    bool blockOnSolidHit = true;
    bool destroyOwnerOnDeactivate = false;
    bool deactivateOnFirstHit = true;
    bool pendingDestroyOwner = false;
};

}  // namespace Spark
