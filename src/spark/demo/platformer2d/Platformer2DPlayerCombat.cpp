#include "spark/demo/platformer2d/Platformer2DPlayerCombat.hpp"

#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark::Platformer2D {

void PlayerCombat::TickCooldown(const float deltaSeconds, Spark::DamageableComponent* damageable) noexcept
{
    if (hurtCooldown > 0.0F) {
        hurtCooldown = std::max(0.0F, hurtCooldown - deltaSeconds);
    }
    if (damageable != nullptr) {
        damageable->SetInvulnerable(hurtCooldown > 0.0F);
    }
}

bool PlayerCombat::TryFireOnAttackPressed(
        const bool attackPressedThisFrame,
        const float playerX,
        const float playerY,
        const bool facingLeft,
        Spark::GameObject* instigator,
        BulletPool& playerBullets,
        const BulletProfile& playerBulletProfile) noexcept
{
    if (!attackPressedThisFrame) {
        return false;
    }
    const float dirX = facingLeft ? -1.0F : 1.0F;
    constexpr float dirY = 0.0F;
    return playerBullets.TrySpawn(
            playerX + dirX * (Config::kPlayerHalfW * 0.75F),
            playerY + Config::kPlayerHalfH * 0.08F,
            dirX,
            dirY,
            instigator,
            playerBulletProfile);
}

namespace {

bool IsInHierarchy(const Spark::GameObject& node, const Spark::GameObject* root) noexcept
{
    if (root == nullptr) {
        return false;
    }
    for (const Spark::GameObject* walk = &node; walk != nullptr; walk = walk->GetParent()) {
        if (walk == root) {
            return true;
        }
    }
    return false;
}

}  // namespace

void PlayerCombat::OnPlayerDamagedByEnemyBullet(
        Spark::GameObject& hitObject,
        Spark::GameObject* playerRoot,
        const float appliedDamage,
        Spark::Sprite2DCharacterAnimFsmComponent* animFsm,
        Spark::GameObject* audioActor,
        const Spark::SharedPtr<Spark::SoundClip>& hurtClip) noexcept
{
    if (appliedDamage <= 0.0F || !IsInHierarchy(hitObject, playerRoot)) {
        return;
    }
    hurtCooldown = Config::kPlayerHurtCooldownSeconds;
    if (animFsm != nullptr) {
        animFsm->RequestHurt();
    }
    if (audioActor != nullptr && hurtClip.Get() != nullptr) {
        DemoAudio::QueueCue(*audioActor, hurtClip, 0.95F);
    }
}

void PlayerCombat::ClearIncomingProjectiles(BulletPool& enemyBullets) noexcept
{
    enemyBullets.DeactivateAll();
    hurtCooldown = 0.0F;
}

}  // namespace Spark::Platformer2D
