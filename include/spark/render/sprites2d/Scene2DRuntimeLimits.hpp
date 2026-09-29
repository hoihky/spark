#pragma once

#include <cstdint>

namespace Spark {

/**
 * Documented 2D scene submit limits (keep in sync with <c>SceneRenderParams</c> caps).
 * External editors should treat these as hard budgets unless a platform build raises them.
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
