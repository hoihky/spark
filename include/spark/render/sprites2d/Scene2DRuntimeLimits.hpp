#pragma once

#include <cstdint>

namespace Spark {

/**
 * Documented 2D scene submit limits (mirrored on <c>SceneRenderParams</c>; validated in <c>Scene2DSubmitCaps</c>).
 * Treat as hard per-frame budgets unless a platform build raises them. Past ~8k sprites or huge tile counts,
 * use tilemaps + culling first; batching / instancing APIs are planned (see <c>SCENE_AND_RENDERING_GAPS.md</c> §3.6).
 */
struct Scene2DRuntimeLimits {
    static constexpr std::uint32_t MaxSprites = 8192U;
    static constexpr std::uint32_t MaxTilemapDraws = 64U;
    static constexpr std::uint32_t MaxTilemapTiles = 65536U;
    static constexpr std::uint32_t MaxParticles = 8192U;
    static constexpr std::uint32_t MaxUiTextures = 16U;
    /** Scene texture array layers for sprite/tile atlases in one frame. */
    static constexpr std::uint32_t MaxSceneTextures = 16U;
    static constexpr std::uint32_t MaxScene2DCompositeViews = 4U;
};

}  // namespace Spark
