#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"

#include <cstdint>

namespace Spark {

class Tileset;

/**
 * Well-known Tiled property names applied to <c>TileDefinition</c> at import time.
 *
 * | Property | Values |
 * |----------|--------|
 * | <c>spark_walkable</c> | <c>true</c> / <c>false</c> — no collision, pathfinding open |
 * | <c>spark_collision</c> | <c>none</c>, <c>full</c>, <c>bottom_half</c>, <c>top_half</c> |
 * | <c>spark_blocks_pathfinding</c> | <c>true</c> / <c>false</c> |
 */
namespace TileGameplayPropertyNames {
inline constexpr const char* kWalkable = "spark_walkable";
inline constexpr const char* kCollision = "spark_collision";
inline constexpr const char* kBlocksPathfinding = "spark_blocks_pathfinding";
}  // namespace TileGameplayPropertyNames

/** Applies a single property list to one atlas tile definition (by atlas index). */
void ApplyTileGameplayPropertiesToDefinition(
        TileDefinition& definition,
        const TilemapPropertyList& properties) noexcept;

/**
 * Applies per-tile TSX properties onto <c>tileset</c> definitions (atlas indices).
 * Tiles without explicit properties are left unchanged.
 */
void ApplyTileGameplayPropertiesFromTilesetDocument(
        Tileset& tileset,
        const TilemapDocumentTileset& documentTileset) noexcept;

[[nodiscard]] bool TilemapPropertyListGetBool(
        const TilemapPropertyList& properties,
        const char* key,
        bool defaultValue) noexcept;

[[nodiscard]] const char* TilemapPropertyListGetString(
        const TilemapPropertyList& properties,
        const char* key,
        const char* defaultValue) noexcept;

}  // namespace Spark
