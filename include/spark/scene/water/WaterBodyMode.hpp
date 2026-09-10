#pragma once

#include <cstdint>

namespace Spark {

/** How a `WaterBodyComponent` occupies world space (simulation/render semantics). */
enum class WaterBodyMode : std::uint8_t {
    /** Camera-centered clipmap tile; mesh recenters when the view moves beyond a threshold. */
    InfiniteOcean = 0,
    /** Fixed axis-aligned rectangle on XZ, sized by {@link WaterBodyExtent}. */
    FiniteLake = 1,
    /** Spline-following river (data-only in v1; renders as a finite strip placeholder). */
    RiverSpline = 2,
};

}  // namespace Spark
