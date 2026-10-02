#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"

namespace Spark {

/**
 * Selects chunk coordinates around a center cell (F2-02).
 * <c>ringRadiusChunks</c> 1 → 3×3, 2 → 5×5.
 */
class GrassChunkNeighborhood {
public:
    void SetRingRadiusChunks(int radius) noexcept { ringRadiusChunks = radius >= 1 ? radius : 1; }
    [[nodiscard]] int GetRingRadiusChunks() const noexcept { return ringRadiusChunks; }

    void BuildAround(const GrassChunkCoordinate& center, Array<GrassChunkCoordinate>& outCoords) const;

private:
    int ringRadiusChunks = 1;
};

}  // namespace Spark
