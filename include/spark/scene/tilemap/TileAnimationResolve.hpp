#pragma once

#include "spark/scene/tilemap/Tileset.hpp"

#include <cstdint>

namespace Spark {

/** Resolves painted tile ids to the atlas frame for a given animation clock. */
class TileAnimationResolver final {
public:
    [[nodiscard]] std::uint16_t ResolveDisplayTileId(
            const Tileset& tileset,
            std::uint16_t sourceTileId,
            float animationTimeSeconds) const noexcept;
};

}  // namespace Spark
