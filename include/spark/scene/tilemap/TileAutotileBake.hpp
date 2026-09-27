#pragma once

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"

namespace Spark {

/** Recomputes autotile display tiles from painted terrain ids. */
class TilemapAutotileBaker final {
public:
    void RebuildLayer(TilemapComponent& tilemap, std::uint32_t layerIndex) const noexcept;

    void RebuildLayerRegion(
            TilemapComponent& tilemap,
            std::uint32_t layerIndex,
            const TilemapCellRegion& region) const noexcept;
};

}  // namespace Spark
