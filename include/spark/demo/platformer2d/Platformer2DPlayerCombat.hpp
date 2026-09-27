#pragma once

#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"
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
    void TickCooldown(float deltaSeconds, Spark::DamageableComponent* damageable = nullptr) noexcept;

    [[nodiscard]] bool CanTakeHit() const noexcept { return hurtCooldown <= 0.0F; }

    [[nodiscard]] bool TryFireOnAttackPressed(
            bool attackPressedThisFrame,
            float playerX,
            float playerY,
            bool facingLeft,
            Spark::GameObject* instigator,
            BulletPool& playerBullets,
            const BulletProfile& playerBulletProfile) noexcept;

    /** Presentation / cooldown after ECS projectile damage to the player hierarchy. */
    void OnPlayerDamagedByEnemyBullet(
            Spark::GameObject& hitObject,
            Spark::GameObject* playerRoot,
            float appliedDamage,
            Spark::Sprite2DCharacterAnimFsmComponent* animFsm,
            Spark::GameObject* audioActor,
            const Spark::SharedPtr<Spark::SoundClip>& hurtClip) noexcept;

    void ClearIncomingProjectiles(BulletPool& enemyBullets) noexcept;

private:
    float hurtCooldown = 0.0F;
};

}  // namespace Spark::Platformer2D
