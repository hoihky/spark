#pragma once

#include "spark/scene/tilemap/Tileset.hpp"

#include <cstdint>

namespace Spark {

/** Resolves a painted tile id to the atlas id for the current animation time. */
[[nodiscard]] std::uint16_t ResolveAnimatedTileId(
        const Tileset& tileset,
        std::uint16_t sourceTileId,
        float animationTimeSeconds) noexcept;

}  // namespace Spark
