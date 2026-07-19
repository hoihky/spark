#include "spark/demo/platformer2d/Platformer2DEnemySquad.hpp"

#include "spark/demo/platformer2d/Platformer2DCombatMath.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/physics/CollisionFilter2D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Platformer2D {

void EnemySquad::Load(
        Spark::GameWorld& world,
        const Spark::SharedPtr<Spark::Texture2D>& enemyAtlas,
        const Spark::Vector4& idleUvIn,
        const Spark::Vector4& attackUvIn)
{
    Unload(world);
    idleUv = idleUvIn;
    attackUv = attackUvIn;
    defeatedCount = 0;
    enemies.Clear();
    enemies.Resize(static_cast<std::size_t>(Config::kEnemyCount));
    for (int ei = 0; ei < Config::kEnemyCount; ++ei) {
        const float ex = Config::kEnemySpawns[static_cast<std::size_t>(ei)][0];
        const float ey = Config::kEnemySpawns[static_cast<std::size_t>(ei)][1];
        const float pMin = Config::kEnemySpawns[static_cast<std::size_t>(ei)][2];
        const float pMax = Config::kEnemySpawns[static_cast<std::size_t>(ei)][3];
        Spark::GameObject* ego = world.CreateGameObject();
        ego->GetName() = Spark::Utf8String("PlatEnemy");
        Spark::TransformComponent* etr = ego->AddComponent<Spark::TransformComponent>();
        etr->SetTranslation({ex, ey, 0.045F + 0.0002F * static_cast<float>(ei)});
        etr->SetScale({Config::kEnemyDrawScale, Config::kEnemyDrawScale, 1.0F});
        Spark::SpriteComponent* espr = ego->AddComponent<Spark::SpriteComponent>(
                enemyAtlas,
                Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                idleUv,
                710 + ei);
        Spark::CircleCollider2DComponent* enemyHit = ego->AddComponent<Spark::CircleCollider2DComponent>(0.68F);
        enemyHit->SetIsTrigger(true);
        enemyHit->SetCategoryBits(Config::kEnemyHurtboxCategoryBits);
        enemyHit->SetMaskBits(Spark::CollisionFilter2D::AllLayersMask());
        ego->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
        enemies[static_cast<std::size_t>(ei)] = {
                true,
                ego,
                etr,
                espr,
                pMin,
                pMax,
                1.0F,
                0.85F + 0.4F * static_cast<float>(ei),
                0.0F,
                1.7F * static_cast<float>(ei),
                ey};
    }
}

void EnemySquad::Unload(Spark::GameWorld& world) noexcept
{
    for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
        if (enemies[ei].go != nullptr) {
            world.DestroyGameObject(enemies[ei].go);
        }
    }
    enemies.Clear();
    defeatedCount = 0;
}

bool EnemySquad::TrySpawnEnemyBullet(
        Enemy& enemy,
        const float playerX,
        const float playerY,
        BulletPool& enemyBullets,
        const BulletProfile& profile) noexcept
{
    if (!enemy.alive || enemy.tr == nullptr) {
        return false;
    }
    const Spark::Vector3 epos = enemy.tr->GetLocalTransform().translation;
    float dx = playerX - epos.x;
    float dy = playerY - epos.y;
    float dirX = 0.0F;
    float dirY = 0.0F;
    CombatMath::NormalizeOrDefault(dx, dy, enemy.patrolDir, 0.0F, dirX, dirY);
    const bool spawned = enemyBullets.TrySpawn(
            epos.x + dirX * (Config::kEnemyHalfW * 0.95F),
            epos.y + dirY * (Config::kEnemyHalfW * 0.35F) + Config::kEnemyHalfH * 0.08F,
            dirX,
            dirY,
            profile);
    if (spawned) {
        enemy.attackFlashTimer = 0.24F;
    }
    return spawned;
}

void EnemySquad::Tick(
        const float deltaSeconds,
        const float sceneTime,
        const float playerX,
        const float playerY,
        BulletPool& enemyBullets,
        const BulletProfile& enemyBulletProfile) noexcept
{
    for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
        Enemy& enemy = enemies[ei];
        if (!enemy.alive || enemy.tr == nullptr) {
            continue;
        }
        Spark::Vector3 pos = enemy.tr->GetLocalTransform().translation;
        pos.x += enemy.patrolDir * Config::kEnemyPatrolSpeed * deltaSeconds;
        if (pos.x <= enemy.patrolMinX) {
            pos.x = enemy.patrolMinX;
            enemy.patrolDir = 1.0F;
        } else if (pos.x >= enemy.patrolMaxX) {
            pos.x = enemy.patrolMaxX;
            enemy.patrolDir = -1.0F;
        }
        const float bob = std::sin(sceneTime * 3.2F + enemy.bobPhase) * Config::kEnemyBobAmplitude;
        pos.y = enemy.baseY + bob;
        enemy.tr->SetTranslation(pos);
        const float sx = enemy.patrolDir < 0.0F ? -Config::kEnemyDrawScale : Config::kEnemyDrawScale;
        enemy.tr->SetScale({sx, Config::kEnemyDrawScale, 1.0F});

        if (enemy.spr != nullptr) {
            if (enemy.attackFlashTimer > 0.0F) {
                enemy.attackFlashTimer = std::max(0.0F, enemy.attackFlashTimer - deltaSeconds);
                enemy.spr->SetUvRect(attackUv);
            } else {
                enemy.spr->SetUvRect(idleUv);
            }
        }

        enemy.fireCooldown -= deltaSeconds;
        if (enemy.fireCooldown > 0.0F) {
            continue;
        }
        const float dx = playerX - pos.x;
        const float dy = playerY - pos.y;
        if (std::fabs(dx) > Config::kEnemyShootRangeX || std::fabs(dy) > Config::kEnemyShootRangeY) {
            continue;
        }
        const bool playerAhead =
                (enemy.patrolDir > 0.0F && dx > 0.75F) || (enemy.patrolDir < 0.0F && dx < -0.75F);
        if (!playerAhead) {
            continue;
        }
        if (TrySpawnEnemyBullet(enemy, playerX, playerY, enemyBullets, enemyBulletProfile)) {
            enemy.fireCooldown = 2.35F + 0.45F * static_cast<float>(ei % 3U);
        } else {
            enemy.fireCooldown = 0.45F;
        }
    }
}

int EnemySquad::ResolvePlayerBulletHits(
        BulletPool& playerBullets,
        ExplosionFx& explosions,
        Spark::GameWorld& world) noexcept
{
    int killed = 0;
    for (std::size_t bi = 0; bi < playerBullets.Slots().GetSize(); ++bi) {
        BulletPool::Slot& bullet = playerBullets.Slots()[bi];
        if (!bullet.active) {
            continue;
        }
        for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
            Enemy& enemy = enemies[ei];
            if (!enemy.alive || enemy.tr == nullptr) {
                continue;
            }
            const Spark::Vector3 epos = enemy.tr->GetLocalTransform().translation;
            if (!CombatMath::BoxOverlap(
                        bullet.cx,
                        bullet.cy,
                        bullet.profile.halfW,
                        bullet.profile.halfH,
                        epos.x,
                        epos.y,
                        Config::kEnemyHalfW,
                        Config::kEnemyHalfH)) {
                continue;
            }
            BulletPool::DeactivateSlot(bullet);

            explosions.SpawnBurst(epos.x, epos.y, Config::kExplosionBurstCount);
            enemy.alive = false;
            world.DestroyGameObject(enemy.go);
            enemy.go = nullptr;
            enemy.tr = nullptr;
            enemy.spr = nullptr;
            ++defeatedCount;
            ++killed;
            break;
        }
    }
    return killed;
}

}  // namespace Spark::Platformer2D
