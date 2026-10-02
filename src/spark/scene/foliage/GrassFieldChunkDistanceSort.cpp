#include "spark/scene/foliage/GrassFieldChunkDistanceSort.hpp"

#include <cmath>

namespace Spark {

namespace {

float ChunkCenterDistanceSq(
        const GrassChunkCoordinate& coord,
        const float viewX,
        const float viewZ,
        const float chunkSize) noexcept {
    const float cx = coord.WorldCenterX(chunkSize);
    const float cz = coord.WorldCenterZ(chunkSize);
    const float dx = cx - viewX;
    const float dz = cz - viewZ;
    return dx * dx + dz * dz;
}

}  // namespace

void GrassFieldChunkDistanceSort::SortByViewDistance(
        Array<GrassChunkCoordinate>& coords,
        const float viewWorldX,
        const float viewWorldZ,
        const float chunkSizeMeters) noexcept {
    const std::size_t n = coords.GetSize();
    if (n < 2U) {
        return;
    }
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t best = i;
        float bestDist = ChunkCenterDistanceSq(coords[i], viewWorldX, viewWorldZ, chunkSizeMeters);
        for (std::size_t j = i + 1U; j < n; ++j) {
            const float dist = ChunkCenterDistanceSq(coords[j], viewWorldX, viewWorldZ, chunkSizeMeters);
            if (dist < bestDist) {
                bestDist = dist;
                best = j;
            }
        }
        if (best != i) {
            const GrassChunkCoordinate tmp = coords[i];
            coords[i] = coords[best];
            coords[best] = tmp;
        }
    }
}

}  // namespace Spark
