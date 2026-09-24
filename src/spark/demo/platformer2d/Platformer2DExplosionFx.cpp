#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"

#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark::Platformer2D {

void ExplosionFx::Initialize(Spark::GameWorld& worldIn) noexcept {
    world = &worldIn;
}

void ExplosionFx::Shutdown() noexcept {
    if (world != nullptr) {
        world->GetVfxSubsystem().Shutdown(*world);
    }
    world = nullptr;
}

void ExplosionFx::QueueAt(const char* const key, const float worldX, const float worldY, const float z) noexcept {
    if (world == nullptr || key == nullptr || key[0] == '\0') {
        return;
    }
    world->GetVfxSubsystem().Queue(key, Spark::Vector3{worldX, worldY, z});
}

void ExplosionFx::SpawnBurst(const float worldX, const float worldY, const int /*particleCount*/) noexcept {
    QueueAt("explosion", worldX, worldY, spawnZ);
}

void ExplosionFx::SpawnEnemyDefeat(const float worldX, const float worldY) noexcept {
    QueueAt("impact", worldX, worldY, spawnZ + 0.01F);
}

void ExplosionFx::SpawnMeleeDefeat(const float worldX, const float worldY) noexcept {
    QueueAt("impact", worldX, worldY, spawnZ + 0.01F);
}

void ExplosionFx::SpawnGemPickup(const float worldX, const float worldY) noexcept {
    QueueAt("loot_sparkle", worldX, worldY, spawnZ + 0.02F);
}

void ExplosionFx::SpawnLandDust(const float /*worldX*/, const float /*worldY*/) noexcept {
    // Intentionally quiet — land dust stacked with other combat VFX and read as stray noise.
}

void ExplosionFx::SpawnMuzzleFlash(const float /*worldX*/, const float /*worldY*/) noexcept {
    // Muzzle flash omitted in the teaching demo to keep the scene readable.
}

void ExplosionFx::SpawnPlayerHurt(const float worldX, const float worldY) noexcept {
    QueueAt("impact", worldX, worldY, spawnZ + 0.01F);
}

void ExplosionFx::SpawnGoalCelebration(const float worldX, const float worldY) noexcept {
    QueueAt("level_up", worldX, worldY + 0.6F, spawnZ + 0.05F);
    QueueAt("confetti", worldX, worldY + 1.2F, spawnZ + 0.06F);
    QueueAt("fireworks", worldX, worldY + 1.8F, spawnZ + 0.07F);
}

}  // namespace Spark::Platformer2D
