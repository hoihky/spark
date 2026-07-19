#pragma once

#include "spark/core/Array.hpp"
#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"
#include "spark/demo/platformer2d/Platformer2DConfig.hpp"
#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark::Platformer2D {

/**
 * Owns patrolling shooter enemies. Separates AI/update from the demo shell so students can study one concern at a time.
 */
class EnemySquad final {
public:
    struct Enemy {
        bool alive = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        float patrolMinX = 0.0F;
        float patrolMaxX = 0.0F;
        float patrolDir = 1.0F;
        float fireCooldown = 0.0F;
        float attackFlashTimer = 0.0F;
        float bobPhase = 0.0F;
        float baseY = 0.0F;
    };

    void Load(
            Spark::GameWorld& world,
            const Spark::SharedPtr<Spark::Texture2D>& enemyAtlas,
            const Spark::Vector4& idleUv,
            const Spark::Vector4& attackUv);

    void Unload(Spark::GameWorld& world) noexcept;

    void Tick(
            float deltaSeconds,
            float sceneTime,
            float playerX,
            float playerY,
            BulletPool& enemyBullets,
            const BulletProfile& enemyBulletProfile) noexcept;

    /** Returns how many enemies were destroyed this call. Spawns explosion FX for each kill. */
    int ResolvePlayerBulletHits(BulletPool& playerBullets, ExplosionFx& explosions, Spark::GameWorld& world) noexcept;

    [[nodiscard]] int GetDefeatedCount() const noexcept { return defeatedCount; }
    [[nodiscard]] int GetTotalCount() const noexcept { return Config::kEnemyCount; }
    [[nodiscard]] Spark::Array<Enemy>& Enemies() noexcept { return enemies; }

private:
    [[nodiscard]] bool TrySpawnEnemyBullet(
            Enemy& enemy,
            float playerX,
            float playerY,
            BulletPool& enemyBullets,
            const BulletProfile& profile) noexcept;

    Spark::Array<Enemy> enemies{};
    Spark::Vector4 idleUv{};
    Spark::Vector4 attackUv{};
    int defeatedCount = 0;
};

}  // namespace Spark::Platformer2D
