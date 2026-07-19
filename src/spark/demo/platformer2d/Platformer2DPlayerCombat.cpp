#include "spark/demo/platformer2d/Platformer2DPlayerCombat.hpp"

#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark::Platformer2D {

void PlayerCombat::TickCooldown(const float deltaSeconds) noexcept
{
    if (hurtCooldown > 0.0F) {
        hurtCooldown = std::max(0.0F, hurtCooldown - deltaSeconds);
    }
}

void PlayerCombat::TryFireOnAttackPressed(
        const bool attackPressedThisFrame,
        const float playerX,
        const float playerY,
        const bool facingLeft,
        BulletPool& playerBullets,
        const BulletProfile& playerBulletProfile) noexcept
{
    if (!attackPressedThisFrame) {
        return;
    }
    const float dirX = facingLeft ? -1.0F : 1.0F;
    constexpr float dirY = 0.0F;
    (void)playerBullets.TrySpawn(
            playerX + dirX * (Config::kPlayerHalfW * 0.75F),
            playerY + Config::kPlayerHalfH * 0.08F,
            dirX,
            dirY,
            playerBulletProfile);
}

void PlayerCombat::ResolveEnemyBulletHits(
        BulletPool& enemyBullets,
        const float playerX,
        const float playerY,
        Spark::HealthComponent* health,
        Spark::DamageableComponent* damageable,
        Spark::Sprite2DCharacterAnimFsmComponent* animFsm,
        Spark::GameObject* audioActor,
        const Spark::SharedPtr<Spark::SoundClip>& hurtClip) noexcept
{
    for (std::size_t bi = 0; bi < enemyBullets.Slots().GetSize(); ++bi) {
        BulletPool::Slot& bullet = enemyBullets.Slots()[bi];
        if (!bullet.active) {
            continue;
        }
        if (!CombatMath::BoxOverlap(
                    bullet.cx,
                    bullet.cy,
                    bullet.profile.halfW,
                    bullet.profile.halfH,
                    playerX,
                    playerY,
                    Config::kPlayerHalfW,
                    Config::kPlayerHalfH)) {
            continue;
        }
        BulletPool::DeactivateSlot(bullet);
        if (hurtCooldown > 0.0F) {
            continue;
        }
        hurtCooldown = Config::kPlayerHurtCooldownSeconds;
        if (animFsm != nullptr) {
            animFsm->RequestHurt();
        }
        if (damageable != nullptr) {
            damageable->ApplyDamage(Config::kEnemyBulletDamage, nullptr);
        } else if (health != nullptr) {
            health->ApplyDamage(Config::kEnemyBulletDamage, nullptr);
        }
        if (audioActor != nullptr && hurtClip.Get() != nullptr) {
            DemoAudio::QueueCue(*audioActor, hurtClip, 0.95F);
        }
    }
}

void PlayerCombat::ClearIncomingProjectiles(BulletPool& enemyBullets) noexcept
{
    enemyBullets.DeactivateAll();
    hurtCooldown = 0.0F;
}

}  // namespace Spark::Platformer2D
