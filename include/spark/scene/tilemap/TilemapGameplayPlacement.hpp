#pragma once

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

#include <cstddef>

namespace Spark {

class Camera2D;
class TilemapComponent;

/** Layer flags used by Kenney sample maps and similar TMX (Objects off grid, Carts decorative). */
void ApplyDefaultGameplayLayerFlags(TilemapComponent& tilemap) noexcept;

[[nodiscard]] bool IsWalkableMapCell(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        std::int32_t x,
        std::int32_t y) noexcept;

/** 4-connected flood fill on <c>walk</c> from <c>start</c> (start must be walkable). */
void CollectReachableWalkableCells(
        const IGridWalkability& walk,
        const GridPathfinder::Cell& start,
        Array<GridPathfinder::Cell>& out) noexcept;

/**
 * Picks a spawn cell in the largest walkable connected component, choosing the cell closest to
 * <c>hintWorldXY</c> within that region.
 */
[[nodiscard]] bool PickSpawnInLargestWalkableRegion(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        const Vector2& hintWorldXY,
        std::size_t minReachableCells,
        GridPathfinder::Cell& outCell) noexcept;

void SortCellsByDistanceFromWorld(
        Array<GridPathfinder::Cell>& cells,
        const TilemapGridFrame& frame,
        const Vector2& worldXY) noexcept;

/** Screen pick (framebuffer pixels, Y down) → grid cell if walkable. */
[[nodiscard]] bool TryPickWalkableGridCellFromScreen(
        const Camera2D& camera,
        const TilemapGridFrame& frame,
        const IGridWalkability& walk,
        float framebufferWidth,
        float framebufferHeight,
        float cursorX,
        float cursorY,
        GridPathfinder::Cell& outCell) noexcept;

}  // namespace Spark
