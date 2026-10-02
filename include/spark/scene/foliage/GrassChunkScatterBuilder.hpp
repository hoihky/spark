#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"
#include "spark/scene/foliage/GrassFieldBounds.hpp"
#include "spark/scene/foliage/GrassTerrainSurfaceProbe.hpp"

namespace Spark {

/** Parameters for one scatter pass (owned by <c>GrassFieldComponent</c>, passed by reference). */
class GrassChunkScatterSettings {
public:
    void SetDensityPerSquareMeter(float value) noexcept { densityPerSquareMeter = value > 0.1F ? value : 0.1F; }
    [[nodiscard]] float GetDensityPerSquareMeter() const noexcept { return densityPerSquareMeter; }

    void SetMaxSlopeDegrees(float degrees) noexcept { maxSlopeDegrees = degrees > 0.0F ? degrees : 45.0F; }
    [[nodiscard]] float GetMaxSlopeDegrees() const noexcept { return maxSlopeDegrees; }

    void SetMaxViewDistanceMeters(float meters) noexcept { maxViewDistanceMeters = meters > 4.0F ? meters : 4.0F; }
    [[nodiscard]] float GetMaxViewDistanceMeters() const noexcept { return maxViewDistanceMeters; }

    /** Outer fraction of view radius used for screen-space dither fade (F2-07). */
    void SetDistanceFadeOuterFraction(float fraction) noexcept {
        distanceFadeOuterFraction = fraction > 0.05F ? (fraction < 0.85F ? fraction : 0.85F) : 0.05F;
    }
    [[nodiscard]] float GetDistanceFadeOuterFraction() const noexcept { return distanceFadeOuterFraction; }

    [[nodiscard]] float GetDistanceFadeStartMeters() const noexcept;
    [[nodiscard]] float GetDistanceFadeEndMeters() const noexcept { return maxViewDistanceMeters; }

    void SetChunkSizeMeters(float meters) noexcept { chunkSizeMeters = meters > 4.0F ? meters : 32.0F; }
    [[nodiscard]] float GetChunkSizeMeters() const noexcept { return chunkSizeMeters; }

    void SetAlbedoTint(const Vector3& rgb) noexcept { albedoTint = rgb; }
    [[nodiscard]] Vector3 GetAlbedoTint() const noexcept { return albedoTint; }

    /** 1 = one blade per cell; 2 = adds a staggered half-cell sample for fuller cover. */
    void SetSamplesPerCell(int count) noexcept { samplesPerCell = count >= 2 ? 2 : 1; }
    [[nodiscard]] int GetSamplesPerCell() const noexcept { return samplesPerCell; }

    /**
     * Per-field instance cap before scene <c>maxGrassInstancesPerFrame</c> (F2-08).
     * Nearest blades kept first when over budget.
     */
    void SetMaxVisibleInstances(std::uint32_t count) noexcept {
        maxVisibleInstances = count > 256U ? count : 256U;
    }
    [[nodiscard]] std::uint32_t GetMaxVisibleInstances() const noexcept { return maxVisibleInstances; }

    /** Limits scatter CPU + chunk cache size (does not change GPU draw count). */
    void SetMaxCachedInstancesPerChunk(std::uint32_t count) noexcept {
        maxCachedInstancesPerChunk = count > 64U ? count : 64U;
    }
    [[nodiscard]] std::uint32_t GetMaxCachedInstancesPerChunk() const noexcept {
        return maxCachedInstancesPerChunk;
    }

    /**
     * When &gt; 0, blades only spawn within this XZ radius from the field origin (soft edge toward the rim).
     * Use with tight <c>GrassFieldBounds</c> to keep meadows localized (F2-04 precursor).
     */
    void SetPlacementRadiusMeters(float meters) noexcept {
        placementRadiusMeters = meters > 0.0F ? meters : 0.0F;
    }
    [[nodiscard]] float GetPlacementRadiusMeters() const noexcept { return placementRadiusMeters; }

private:
    float densityPerSquareMeter = 8.0F;
    int samplesPerCell = 1;
    std::uint32_t maxVisibleInstances = 8000U;
    std::uint32_t maxCachedInstancesPerChunk = 1400U;
    float maxSlopeDegrees = 48.0F;
    float maxViewDistanceMeters = 72.0F;
    float distanceFadeOuterFraction = 0.28F;
    float chunkSizeMeters = 32.0F;
    float placementRadiusMeters = 0.0F;
    Vector3 albedoTint{0.52F, 0.92F, 0.38F};
};

/** Fills instance records for one chunk using jittered XZ scatter (F2-03). */
class GrassChunkScatterBuilder {
public:
    void ScatterChunk(
            const GrassChunkCoordinate& chunk,
            const GrassChunkScatterSettings& settings,
            const GrassFieldBounds& bounds,
            const Vector3& fieldOriginWorld,
            const GrassTerrainSurfaceProbe& terrainProbe,
            Array<FoliageInstanceRecord>& outInstances) const;

private:
    [[nodiscard]] static float Hash01(std::uint32_t seed) noexcept;

    bool TryPlaceBlade(
            float worldX,
            float worldZ,
            std::uint32_t chunkSeed,
            std::uint32_t placementSeed,
            const GrassChunkScatterSettings& settings,
            const GrassFieldBounds& bounds,
            const Vector3& fieldOriginWorld,
            const GrassTerrainSurfaceProbe& terrainProbe,
            Array<FoliageInstanceRecord>& outInstances) const;
};

}  // namespace Spark
