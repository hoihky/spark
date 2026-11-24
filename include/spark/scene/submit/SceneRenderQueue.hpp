#pragma once

#include <cstdint>

namespace Spark {

/**
 * Coarse draw bucket for lit mesh submission (sprites/tilemaps use the same layer registry via <c>DrawableSortKey</c>).
 * Finer ordering within a bucket uses <c>SceneDrawItem::drawableSort</c>.
 */
enum class SceneRenderQueue : std::uint8_t {
    /** Registered <c>Background</c> sorting layer (draws before default opaque meshes). */
    Background = 0,
    /** Default HDR opaque subpass. */
    Opaque = 1,
    /** Alpha / transmission path (<c>transparentDraws</c>); may also be inferred from material opacity. */
    Transparent = 2,
    /** Registered <c>Overlay</c> sorting layer (late in opaque list). */
    Overlay = 3,
};

}  // namespace Spark