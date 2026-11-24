#pragma once

#include "spark/render/sprites2d/Scene2DRuntimeLimits.hpp"

namespace Spark {

struct SceneRenderParams;

/**
 * Single entry point for documented 2D submit budgets (see <c>Scene2DRuntimeLimits</c>).
 * Product code should use these helpers instead of ad-hoc comparisons so spill logging stays consistent.
 */
struct Scene2DSubmitCaps final {
    using Limits = Scene2DRuntimeLimits;

    static constexpr std::uint32_t MaxSprites = Limits::MaxSprites;
    static constexpr std::uint32_t MaxTilemapDraws = Limits::MaxTilemapDraws;
    static constexpr std::uint32_t MaxTilemapTiles = Limits::MaxTilemapTiles;
    static constexpr std::uint32_t MaxParticles = Limits::MaxParticles;

    [[nodiscard]] static bool HasSpriteCapacity(const SceneRenderParams& params) noexcept;
    [[nodiscard]] static bool HasTilemapDrawCapacity(const SceneRenderParams& params) noexcept;
    [[nodiscard]] static std::uint32_t RemainingTilemapTiles(const SceneRenderParams& params) noexcept;
    [[nodiscard]] static bool HasParticleCapacity(const SceneRenderParams& params) noexcept;

    /** Logs once per process per cap kind (see <c>SCENE_AND_RENDERING_GAPS.md</c> §3.6). */
    static void NotifySpriteCapacityReached() noexcept;
    static void NotifyTilemapDrawCapacityReached() noexcept;
    static void NotifyTilemapTileCapacityReached() noexcept;
    static void NotifyParticleCapacityReached() noexcept;
};

}  // namespace Spark