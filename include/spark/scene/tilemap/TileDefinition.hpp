#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"

#include "spark/scene/tilemap/TileAnimation.hpp"

#include <cstdint>

namespace Spark {

/** How a tile participates in <c>TilemapCollider2D</c> baking. */
enum class TileCollisionShape : std::uint8_t {
    /** Same as <c>FullCell</c> (keeps legacy maps solid when definitions are default). */
    InheritFullCell = 0,
    /** Drawn tile with no physics shape (decor / visual-only). */
    None = 1,
    /** Axis-aligned box covering the whole cell. */
    FullCell = 2,
    /** Bottom half of the cell (+Y is up in local tile space). */
    BottomHalf = 3,
    /** Top half of the cell. */
    TopHalf = 4,
    /** Convex polygon in normalized cell space [0,1]×[0,1]. */
    CustomConvex = 5,
};

/** Optional gameplay flags (extensible without changing collision). */
enum class TileDefinitionFlags : std::uint16_t {
    None = 0,
    /** Cell blocks pathfinding even when collision is disabled. */
    BlocksPathfinding = 1U << 0,
    /** Cell is walkable even when collision would block. */
    ForceWalkable = 1U << 1,
};

[[nodiscard]] constexpr TileDefinitionFlags operator|(
        const TileDefinitionFlags a,
        const TileDefinitionFlags b) noexcept {
    return static_cast<TileDefinitionFlags>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}

/** Normalized anchor within the cell (0–1). Default center; used for render offset and Y-sort bias. */
struct TileAnchor {
    float normalizedX = 0.5F;
    float normalizedY = 0.5F;
    /** Added to world Y when resolving sort keys (tall props). */
    float sortYOffsetWorld = 0.0F;
};

static constexpr std::uint32_t kMaxTileCustomCollisionVertices = 8U;

/**
 * Per-atlas-tile metadata. Indexed by tile id (0 … cells−1).
 * Default-constructed entries behave like legacy full-cell collision.
 */
struct TileDefinition {
    TileCollisionShape collisionShape = TileCollisionShape::InheritFullCell;
    TileDefinitionFlags flags = TileDefinitionFlags::None;
    TileAnchor anchor{};

    /** Index into <c>Tileset</c> animation clips; <c>kNoTileAnimationClip</c> = static tile. */
    std::uint16_t animationClipIndex = kNoTileAnimationClip;
    /**
     * Autotile terrain group (0 = not autotiled). Neighbor connectivity uses matching
     * <c>autotileGroup</c> on painted tiles.
     */
    std::uint8_t autotileGroup = 0;

    /** Used when <c>collisionShape == CustomConvex</c> (clockwise, normalized 0–1). */
    Array<Vector2> customCollisionVertices;

    [[nodiscard]] bool ContributesCollision() const noexcept {
        return collisionShape != TileCollisionShape::None;
    }

    [[nodiscard]] TileCollisionShape EffectiveCollisionShape() const noexcept {
        if (collisionShape == TileCollisionShape::InheritFullCell) {
            return TileCollisionShape::FullCell;
        }
        return collisionShape;
    }

    [[nodiscard]] bool HasFlag(const TileDefinitionFlags bit) const noexcept {
        return (static_cast<std::uint16_t>(flags) & static_cast<std::uint16_t>(bit)) != 0;
    }
};

}  // namespace Spark
