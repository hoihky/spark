#include "spark/scene/submit/Scene2DSubmitCaps.hpp"

#include "spark/engine/SceneRenderParams.hpp"

#include <atomic>
#include <cstdio>

namespace Spark {

static_assert(SceneRenderParams::MaxSprites == Scene2DRuntimeLimits::MaxSprites);
static_assert(SceneRenderParams::MaxTilemapDraws == Scene2DRuntimeLimits::MaxTilemapDraws);
static_assert(SceneRenderParams::MaxTilemapTiles == Scene2DRuntimeLimits::MaxTilemapTiles);
static_assert(SceneRenderParams::MaxParticles == Scene2DRuntimeLimits::MaxParticles);

namespace {

std::atomic<bool> gLoggedSpriteCap{false};
std::atomic<bool> gLoggedTilemapDrawCap{false};
std::atomic<bool> gLoggedTilemapTileCap{false};
std::atomic<bool> gLoggedParticleCap{false};

void LogOnce(std::atomic<bool>& flag, const char* message) noexcept {
    bool expected = false;
    if (flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        std::fprintf(stderr, "%s", message);
    }
}

}  // namespace

bool Scene2DSubmitCaps::HasSpriteCapacity(const SceneRenderParams& params) noexcept {
    return params.sprites.GetSize() < MaxSprites;
}

bool Scene2DSubmitCaps::HasTilemapDrawCapacity(const SceneRenderParams& params) noexcept {
    return params.tilemaps.GetSize() < MaxTilemapDraws;
}

std::uint32_t Scene2DSubmitCaps::RemainingTilemapTiles(const SceneRenderParams& params) noexcept {
    const std::size_t used = params.tilemapTiles.GetSize();
    if (used >= MaxTilemapTiles) {
        return 0U;
    }
    return MaxTilemapTiles - static_cast<std::uint32_t>(used);
}

bool Scene2DSubmitCaps::HasParticleCapacity(const SceneRenderParams& params) noexcept {
    return params.particles.GetSize() < MaxParticles;
}

void Scene2DSubmitCaps::NotifySpriteCapacityReached() noexcept {
    LogOnce(
            gLoggedSpriteCap,
            "Spark: sprite draw limit (8192) reached; extra sprites dropped for this frame. "
            "Use tilemaps, culling, or split composite views before adding batching.\n");
}

void Scene2DSubmitCaps::NotifyTilemapDrawCapacityReached() noexcept {
    LogOnce(
            gLoggedTilemapDrawCap,
            "Spark: tilemap layer draw limit (64) reached; split maps across GameObjects or "
            "enable chunked maps (roadmap).\n");
}

void Scene2DSubmitCaps::NotifyTilemapTileCapacityReached() noexcept {
    LogOnce(
            gLoggedTilemapTileCap,
            "Spark: tilemap tile instance limit (65536) reached; cull layers or stream chunks.\n");
}

void Scene2DSubmitCaps::NotifyParticleCapacityReached() noexcept {
    LogOnce(
            gLoggedParticleCap,
            "Spark: particle instance limit (8192) reached; lower emitter max or pool VFX.\n");
}

}  // namespace Spark