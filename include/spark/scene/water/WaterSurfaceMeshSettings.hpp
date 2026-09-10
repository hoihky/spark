#pragma once

namespace Spark {

/** Tessellation and clipmap behavior for `WaterSurfaceMesh` (owned by `WaterBodyComponent`). */
class WaterSurfaceMeshSettings {
public:
    /** Half-size of one water tile in world units (full span = 2 × tileHalfExtent). */
    [[nodiscard]] float GetTileHalfExtent() const noexcept { return tileHalfExtent; }
    void SetTileHalfExtent(float value) noexcept { tileHalfExtent = value; }

    /** Grid lines per axis on the tile (minimum 2 → 3×3 vertices). */
    [[nodiscard]] int GetSubdivisionsPerAxis() const noexcept { return subdivisionsPerAxis; }
    void SetSubdivisionsPerAxis(int value) noexcept { subdivisionsPerAxis = value; }

    /**
     * Recenters the infinite-ocean tile when the camera moves farther than this distance from the current anchor (XZ).
     */
    [[nodiscard]] float GetRebuildMoveThreshold() const noexcept { return rebuildMoveThreshold; }
    void SetRebuildMoveThreshold(float value) noexcept { rebuildMoveThreshold = value; }

    /**
     * World units along X or Z per one UV repeat. Non-positive values derive from tile span at build time.
     */
    [[nodiscard]] float GetWorldUnitsPerTextureRepeat() const noexcept { return worldUnitsPerTextureRepeat; }
    void SetWorldUnitsPerTextureRepeat(float value) noexcept { worldUnitsPerTextureRepeat = value; }

private:
    float tileHalfExtent = 128.0F;
    int subdivisionsPerAxis = 64;
    float rebuildMoveThreshold = 32.0F;
    float worldUnitsPerTextureRepeat = -1.0F;
};

}  // namespace Spark
