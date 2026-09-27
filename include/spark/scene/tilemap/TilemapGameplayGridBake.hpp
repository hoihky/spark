#pragma once

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace Spark {

/** Builds <c>TilemapGameplayGrid</c> walkability from tilemap layers. */
class TilemapGameplayGridBaker final {
public:
    void BakeFull(
            const TilemapComponent& tilemap,
            TilemapGameplayWalkRule rule,
            TilemapGameplayGrid& outGrid) const;

    void BakeRegion(
            const TilemapComponent& tilemap,
            TilemapGameplayWalkRule rule,
            TilemapGameplayGrid& outGrid,
            const TilemapCellRegion& region) const;
};

}  // namespace Spark
