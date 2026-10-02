#pragma once

#include <cmath>
#include <cstdint>

namespace Spark {

/** Integer cell index for a grass chunk on the world XZ grid (F2-02). */
class GrassChunkCoordinate {
public:
    GrassChunkCoordinate() = default;
    GrassChunkCoordinate(int indexX, int indexZ) noexcept : chunkIndexX(indexX), chunkIndexZ(indexZ) {}

    [[nodiscard]] static GrassChunkCoordinate FromWorldPosition(
            float worldX,
            float worldZ,
            float chunkSizeMeters) noexcept;

    [[nodiscard]] int GetIndexX() const noexcept { return chunkIndexX; }
    [[nodiscard]] int GetIndexZ() const noexcept { return chunkIndexZ; }

    [[nodiscard]] float WorldMinX(float chunkSizeMeters) const noexcept {
        return static_cast<float>(chunkIndexX) * chunkSizeMeters;
    }

    [[nodiscard]] float WorldMinZ(float chunkSizeMeters) const noexcept {
        return static_cast<float>(chunkIndexZ) * chunkSizeMeters;
    }

    [[nodiscard]] float WorldCenterX(float chunkSizeMeters) const noexcept {
        return WorldMinX(chunkSizeMeters) + chunkSizeMeters * 0.5F;
    }

    [[nodiscard]] float WorldCenterZ(float chunkSizeMeters) const noexcept {
        return WorldMinZ(chunkSizeMeters) + chunkSizeMeters * 0.5F;
    }

    [[nodiscard]] bool operator==(const GrassChunkCoordinate& other) const noexcept {
        return chunkIndexX == other.chunkIndexX && chunkIndexZ == other.chunkIndexZ;
    }

private:
    int chunkIndexX = 0;
    int chunkIndexZ = 0;
};

}  // namespace Spark
