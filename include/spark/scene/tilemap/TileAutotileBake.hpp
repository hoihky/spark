#pragma once

#include "spark/ecs/components/rendering/TilemapComponent.hpp"

namespace Spark {

/** Recomputes display <c>tileId</c> / transforms for autotile terrain on one map layer. */
void RebuildTilemapAutotileLayer(TilemapComponent& tilemap, std::uint32_t layerIndex) noexcept;

}  // namespace Spark
