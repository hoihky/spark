#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

/** Key/value custom properties (Tiled-style); serialization-friendly flat list. */
struct TilemapObjectProperty {
    Utf8String key{};
    Utf8String value{};
};

/**
 * How a marker participates at runtime.
 * - <c>Runtime</c>: may spawn when a handler is registered.
 * - <c>GizmoOnly</c>: never spawned; optional debug draw only (editor / dev).
 */
enum class TilemapObjectMarkerMode : std::uint8_t {
    Runtime = 0,
    GizmoOnly = 1,
};

/**
 * One logical object on an object layer (spawn point, chest, warp, path node).
 * Cell coordinates are map-local; use <c>TilemapGridFrame</c> for world space.
 */
struct TilemapObjectMarker {
    std::uint32_t id = 0;
    Utf8String name{};
    Utf8String typeId{};
    std::int32_t cellX = 0;
    std::int32_t cellY = 0;
    /** Normalized offset within the cell (0 = corner, 1 = opposite corner). */
    float offsetX = 0.5F;
    float offsetY = 0.5F;
    TilemapObjectMarkerMode mode = TilemapObjectMarkerMode::Runtime;
    Array<TilemapObjectProperty> properties{};
};

/** Tiled-style object layer: markers only (no tile grid). */
struct TilemapObjectLayer {
    Utf8String name{};
    bool visible = true;
    Array<TilemapObjectMarker> markers{};
};

}  // namespace Spark
