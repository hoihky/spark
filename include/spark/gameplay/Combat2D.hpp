#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class GameplayTeamPolicy;
class Hurtbox2DComponent;

/** One hurtbox hit from a combat overlap query. */
class CombatHurtboxOverlap2D final {
public:
    CombatHurtboxOverlap2D() noexcept = default;
    CombatHurtboxOverlap2D(GameObject* ownerIn, Hurtbox2DComponent* hurtboxIn) noexcept
        : owner(ownerIn), hurtbox(hurtboxIn) {}

    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }
    [[nodiscard]] Hurtbox2DComponent* GetHurtbox() const noexcept { return hurtbox; }

private:
    GameObject* owner = nullptr;
    Hurtbox2DComponent* hurtbox = nullptr;
};

/**
 * Damage and optional hurt reactions for a single 2D combat target (hitboxes, projectiles, zones).
 */
class Combat2DTarget final {
public:
    explicit Combat2DTarget(GameObject& target) noexcept : target(&target) {}

    void SetTeamPolicy(const GameplayTeamPolicy* policy) noexcept { teamPolicy = policy; }

    /**
     * @return Applied damage (0 when blocked or no health receiver).
     */
    [[nodiscard]] float ApplyDamage(GameObject* instigator, float rawDamage) noexcept;

    void RequestHurtReaction() noexcept;

private:
    GameObject* target = nullptr;
    const GameplayTeamPolicy* teamPolicy = nullptr;
};

/** Physics overlap queries narrowed to enabled <c>Hurtbox2DComponent</c> owners. */
class CombatHurtboxWorldQuery2D final {
public:
    void CollectCircleOverlaps(
            GameWorld& world,
            float centerX,
            float centerY,
            float radius,
            const PhysicsQueryFilter2D& filter,
            GameObject* ignoreObject,
            Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept;

    void CollectArcOverlaps(
            GameWorld& world,
            float originX,
            float originY,
            float radius,
            float dirX,
            float dirY,
            float halfAngleRadians,
            const PhysicsQueryFilter2D& filter,
            GameObject* ignoreObject,
            Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept;

    void CollectAabbOverlaps(
            GameWorld& world,
            const CollisionAabb2& worldAabb,
            float dynamicQueryRadius,
            const PhysicsQueryFilter2D& filter,
            GameObject* ignoreObject,
            Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept;

private:
    static void AppendUniqueHurtbox_(
            GameObject* candidate,
            GameObject* ignoreObject,
            Array<CombatHurtboxOverlap2D>& outOverlaps,
            Array<GameObject*>& seenOwners) noexcept;
};

}  // namespace Spark
