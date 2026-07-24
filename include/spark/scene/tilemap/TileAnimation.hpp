#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>

namespace Spark {

struct TileAnimationFrame {
    std::uint16_t tileId = 0;
    float durationSeconds = 0.1F;
};

/** Tiled-style tile animation: sequence of atlas tile ids with per-frame durations. */
struct TileAnimationClip {
    Array<TileAnimationFrame> frames{};
    bool loop = true;
};

static constexpr std::uint16_t kNoTileAnimationClip = 0xFFFFU;

}  // namespace Spark
