#include "spark/scene/foliage/GrassChunkNeighborhood.hpp"

namespace Spark {

void GrassChunkNeighborhood::BuildAround(
        const GrassChunkCoordinate& center,
        Array<GrassChunkCoordinate>& outCoords) const {
    outCoords.Clear();
    const int cx = center.GetIndexX();
    const int cz = center.GetIndexZ();
    for (int dz = -ringRadiusChunks; dz <= ringRadiusChunks; ++dz) {
        for (int dx = -ringRadiusChunks; dx <= ringRadiusChunks; ++dx) {
            outCoords.PushBack(GrassChunkCoordinate{cx + dx, cz + dz});
        }
    }
}

}  // namespace Spark
