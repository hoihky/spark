#pragma once

#include <cstdint>

namespace Spark {

/**
 * Parameters for procedural heightmap terrain (XZ grid, fBM noise).
 * Used by TerrainMeshGenerator and TerrainComponent.
 */
struct TerrainGeneratorSettings {
    std::int32_t subdivX = 96;
    std::int32_t subdivZ = 96;
    float halfExtentX = 56.0F;
    float halfExtentZ = 56.0F;
    float heightScale = 14.0F;
    /** World-space frequency of base noise (larger = more hills per unit). */
    float noiseScale = 0.055F;
    std::int32_t octaves = 6;
    float persistence = 0.48F;
    float lacunarity = 2.05F;
    std::uint32_t seed = 0x7E57C0DEu;
    /**
     * World units along X or Z per one texture repeat (sampler uses REPEAT).
     * Use ~2 for tiled Kenney 64px floors; use 2×halfExtentX for a single procedural span.
     */
    float worldUnitsPerTextureRepeat = 112.0F;
};

}  // namespace Spark
