#pragma once

#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark::Platformer2D {

/**
 * Thin facade over <c>VfxSubsystem</c> for enemy defeat bursts.
 * Replaces the legacy sprite-particle pool with pooled <c>VfxPlayerComponent</c> one-shots.
 */
class ExplosionFx final {
public:
    void Initialize(Spark::GameWorld& world, const char* vfxAssetKey = "explosion") noexcept;
    void Shutdown() noexcept;

    void SpawnBurst(float worldX, float worldY, int /*particleCount*/ = 0) noexcept;

    void Tick(float /*deltaSeconds*/) noexcept {}

private:
    Spark::GameWorld* world = nullptr;
    const char* assetKey = "explosion";
    float spawnZ = 0.08F;
};

}  // namespace Spark::Platformer2D
