#pragma once

#include <cstdint>

namespace Spark {

/** Stable handle into <c>RenderLayerRegistry</c> (index into the registered layer table). */
using RenderLayerId = std::uint16_t;

inline constexpr RenderLayerId kInvalidRenderLayerId = 0xFFFFU;

}  // namespace Spark
