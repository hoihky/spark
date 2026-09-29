#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

class GameWorld;
class Texture2D;
class TilemapGameplayGrid;

/** CPU walkability minimap (GPU RT capture can replace this later). */
struct Scene2DMinimapColors {
    std::uint8_t walkableR = 72U;
    std::uint8_t walkableG = 62U;
    std::uint8_t walkableB = 48U;
    std::uint8_t blockedR = 28U;
    std::uint8_t blockedG = 24U;
    std::uint8_t blockedB = 32U;
};

/** Fills @p texture with one pixel per map cell (nearest upscale on HUD blit). */
void RebuildMinimapTextureFromGameplayGrid(
        const TilemapGameplayGrid& grid,
        Texture2D& texture,
        const Scene2DMinimapColors& colors = {}) noexcept;

/**
 * Draws minimap in the top-right overlay and a player marker dot.
 * Registers @p texture in @p world when needed.
 */
void PatchScene2DMinimapHud(
        SceneRenderParams& params,
        GameWorld& world,
        const SharedPtr<Texture2D>& texture,
        const TilemapGridFrame& frame,
        const Vector2& playerWorldXY,
        float framebufferWidth,
        float framebufferHeight) noexcept;

}  // namespace Spark
