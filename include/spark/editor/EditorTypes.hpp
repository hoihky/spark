#pragma once

#include <cstdint>

namespace Spark::Editor {

/** Edit world authoring vs run simulation (PIE). */
enum class EditorMode : std::uint8_t {
    Edit = 0,
    Play = 1,
};

/** 2D sprite/tilemap vs 3D mesh authoring defaults. */
enum class WorkspaceDimension : std::uint8_t {
    ThreeD = 0,
    TwoD = 1,
};

}  // namespace Spark::Editor
