#pragma once

#include "spark/core/Array.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/math/Vector2.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class GameObject;
class TilemapComponent;
class TilemapGridFrame;
class Tileset;

/** Yellow sand on Kenney <c>sampleMap.tmx</c> (TMX GIDs 1–4, packed atlas 120–123). */
[[nodiscard]] bool IsKenneyTinyDungeonSandFloorTile(const std::uint16_t atlasTileId) noexcept;

/**
 * Walkable floor for collision, grid bake, and P0 spawn/gems (sand, connector 126, town plates 72–77/83).
 * Excludes roof trim 124/125 and brick walls (e.g. atlas 88).
 */
[[nodiscard]] bool IsKenneyTinyDungeonWalkableFloorTile(const std::uint16_t atlasTileId) noexcept;

void ConfigureKenneyTinyDungeonGameplayTileset(Tileset& tileset) noexcept;
void ApplyKenneyTinyDungeonGameplayLayerFlags(TilemapComponent& tilemap) noexcept;

/** Applies Kenney tile definitions and layer flags on the level's <c>TilemapComponent</c>. */
void ApplyKenneyTinyDungeonGameplayToTilemap(GameObject& levelRoot) noexcept;

[[nodiscard]] std::uint32_t FindKenneyDungeonLayerIndex(const TilemapComponent& tilemap) noexcept;

[[nodiscard]] bool IsKenneySandMapCell(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const std::int32_t x,
        const std::int32_t y) noexcept;

void CollectReachableKenneySandCells(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const GridPathfinder::Cell& start,
        Array<GridPathfinder::Cell>& out) noexcept;

/**
 * Spawn on the largest yellow-sand region, at the cell closest to <c>hintWorldXY</c>.
 */
[[nodiscard]] bool PickKenneySandSpawnCell(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const TilemapGridFrame& frame,
        const Vector2& hintWorldXY,
        const std::size_t minReachableCells,
        GridPathfinder::Cell& outCell) noexcept;

}  // namespace Spark
