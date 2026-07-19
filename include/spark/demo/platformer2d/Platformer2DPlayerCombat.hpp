#pragma once

#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"
#include "spark/demo/platformer2d/Platformer2DCombatMath.hpp"
#include "spark/demo/platformer2d/Platformer2DConfig.hpp"
#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {
class GameObject;
}

namespace Spark::Platformer2D {

/**
 * Facade for player weapon + damage reception. Keeps vitality rules in one place (invulnerability window, damage amounts).
 */
class PlayerCombat final {
public:
    void TickCooldown(float deltaSeconds) noexcept;

    [[nodiscard]] bool CanTakeHit() const noexcept { return hurtCooldown <= 0.0F; }

    void TryFireOnAttackPressed(
            bool attackPressedThisFrame,
            float playerX,
            float playerY,
            bool facingLeft,
            BulletPool& playerBullets,
            const BulletProfile& playerBulletProfile) noexcept;

    void ResolveEnemyBulletHits(
            BulletPool& enemyBullets,
            float playerX,
            float playerY,
            Spark::HealthComponent* health,
            Spark::DamageableComponent* damageable,
            Spark::Sprite2DCharacterAnimFsmComponent* animFsm,
            Spark::GameObject* audioActor,
            const Spark::SharedPtr<Spark::SoundClip>& hurtClip) noexcept;

    void ClearIncomingProjectiles(BulletPool& enemyBullets) noexcept;

private:
    float hurtCooldown = 0.0F;
};

}  // namespace Spark::Platformer2D
