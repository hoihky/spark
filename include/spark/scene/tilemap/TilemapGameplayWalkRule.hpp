#pragma once

#include <cstdint>

namespace Spark {

/**
 * How map cells become walkable/blocked when baking a <c>TilemapGameplayGrid</c>.
 */
enum class TilemapGameplayWalkRule : std::uint8_t {
    /**
     * Matches legacy pathfinding samples: any non-empty cell on a gameplay layer is walkable;
     * empty cells are blocked.
     */
    OccupiedWalkable = 0,
    /**
     * Uses <c>TileDefinition</c> collision + <c>TileDefinitionFlags</c> (recommended for walls/decor).
     */
    DefinitionAndFlags = 1,
    /** Blocked when the tile would contribute a physics collider (aligned with <c>TilemapCollider2D</c>). */
    CollisionAligned = 2,
};

}  // namespace Spark
