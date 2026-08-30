#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"

#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark::Platformer2D {

void ExplosionFx::Initialize(Spark::GameWorld& worldIn, const char* const vfxAssetKeyIn) noexcept {
    world = &worldIn;
    assetKey = vfxAssetKeyIn != nullptr && vfxAssetKeyIn[0] != '\0' ? vfxAssetKeyIn : "explosion";
}

void ExplosionFx::Shutdown() noexcept {
    world = nullptr;
}

void ExplosionFx::SpawnBurst(const float worldX, const float worldY, const int /*particleCount*/) noexcept {
    if (world == nullptr || assetKey == nullptr || assetKey[0] == '\0') {
        return;
    }
    world->GetVfxSubsystem().Queue(assetKey, Spark::Vector3{worldX, worldY, spawnZ});
}

}  // namespace Spark::Platformer2D
