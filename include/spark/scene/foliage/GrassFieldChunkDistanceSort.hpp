#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"

namespace Spark {

/** Orders chunk coordinates so the camera-neighborhood fills the instance budget nearest-first. */
class GrassFieldChunkDistanceSort {
public:
    static void SortByViewDistance(
            Array<GrassChunkCoordinate>& coords,
            float viewWorldX,
            float viewWorldZ,
            float chunkSizeMeters) noexcept;
};

}  // namespace Spark
