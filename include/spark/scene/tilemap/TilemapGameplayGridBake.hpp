#pragma once

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace Spark {

/** Fills <c>outGrid</c> from all gameplay layers on <c>tilemap</c>. */
void BakeTilemapGameplayGrid(
        const TilemapComponent& tilemap,
        TilemapGameplayWalkRule rule,
        TilemapGameplayGrid& outGrid);

}  // namespace Spark
