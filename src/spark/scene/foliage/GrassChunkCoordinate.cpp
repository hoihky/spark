#include "spark/scene/foliage/GrassChunkCoordinate.hpp"

#include <cmath>

namespace Spark {

GrassChunkCoordinate GrassChunkCoordinate::FromWorldPosition(
        const float worldX,
        const float worldZ,
        const float chunkSizeMeters) noexcept {
    const float size = chunkSizeMeters > 1.0e-3F ? chunkSizeMeters : 32.0F;
    const int ix = static_cast<int>(std::floor(worldX / size));
    const int iz = static_cast<int>(std::floor(worldZ / size));
    return GrassChunkCoordinate{ix, iz};
}

}  // namespace Spark
