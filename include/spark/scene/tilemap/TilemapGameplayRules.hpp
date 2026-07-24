#pragma once

#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace Spark {

[[nodiscard]] bool TileBlocksGameplayPath(
        const TileCell& cell,
        const TileDefinition& definition,
        TilemapGameplayWalkRule rule) noexcept;

}  // namespace Spark
