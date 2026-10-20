#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/PhysicsQueries3D.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class Hurtbox3DComponent;

/** One hurtbox hit from a combat overlap query. */
class CombatHurtboxOverlap final {
public:
    CombatHurtboxOverlap() noexcept = default;
    CombatHurtboxOverlap(GameObject* ownerIn, Hurtbox3DComponent* hurtboxIn) noexcept
        : owner(ownerIn), hurtbox(hurtboxIn) {}

    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }
    [[nodiscard]] Hurtbox3DComponent* GetHurtbox() const noexcept { return hurtbox; }

private:
    GameObject* owner = nullptr;
    Hurtbox3DComponent* hurtbox = nullptr;
};

/**
 * Damage, knockback, and animation reactions for a single 3D combat target (mirrors the 2D damage path).
 */
class Combat3DTarget final {
public:
    explicit Combat3DTarget(GameObject& target) noexcept : target(&target) {}

    /**
     * Applies damage through hurtbox rules and damageable/health on the hierarchy.
     * @return Applied damage (0 when blocked or no receiver).
     */
    [[nodiscard]] float ApplyDamage(GameObject* instigator, float rawDamage) noexcept;

    void ApplyKnockback(const Vector3& worldDirection, float meters) noexcept;
    void RequestHurtReaction() noexcept;

private:
    GameObject* target = nullptr;
};

/** Layer-filtered hurtbox overlap against world objects. */
class CombatHurtboxSphereQuery final {
public:
    explicit CombatHurtboxSphereQuery(GameWorld& worldIn) noexcept : world(&worldIn) {}

    void SetFilter(const PhysicsQueryFilter3D& filterIn) noexcept { filter = filterIn; }
    [[nodiscard]] const PhysicsQueryFilter3D& GetFilter() const noexcept { return filter; }

    void CollectOverlaps(const Vector3& center, float radius, Array<CombatHurtboxOverlap>& outHits) noexcept;

private:
    GameWorld* world = nullptr;
    PhysicsQueryFilter3D filter{};
};

}  // namespace Spark
