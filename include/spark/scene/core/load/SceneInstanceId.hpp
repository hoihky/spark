#pragma once

#include <cstdint>

namespace Spark {

/** Stable id for a loaded scene instance (additive multi-scene). 0 = untagged / global. */
using SceneInstanceId = std::uint64_t;

inline constexpr SceneInstanceId kInvalidSceneInstanceId = 0;

}  // namespace Spark
