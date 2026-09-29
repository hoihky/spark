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

}  // namespace Spark
