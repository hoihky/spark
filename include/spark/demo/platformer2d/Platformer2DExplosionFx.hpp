#pragma once

#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark::Platformer2D {

/**
 * Thin facade over <c>VfxSubsystem</c> for platformer combat and pickup feedback.
 */
class ExplosionFx final {
public:
    void Initialize(Spark::GameWorld& world) noexcept;
    void Shutdown() noexcept;

    void SpawnBurst(float worldX, float worldY, int /*particleCount*/ = 0) noexcept;
    void SpawnEnemyDefeat(float worldX, float worldY) noexcept;
    void SpawnGemPickup(float worldX, float worldY) noexcept;
    void SpawnLandDust(float worldX, float worldY) noexcept;
    void SpawnMuzzleFlash(float worldX, float worldY) noexcept;
    void SpawnPlayerHurt(float worldX, float worldY) noexcept;
    void SpawnGoalCelebration(float worldX, float worldY) noexcept;

    void Tick(float /*deltaSeconds*/) noexcept {}

private:
    void QueueAt(const char* key, float worldX, float worldY, float z) noexcept;

    Spark::GameWorld* world = nullptr;
    float spawnZ = 0.08F;
};

}  // namespace Spark::Platformer2D
