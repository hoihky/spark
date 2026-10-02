#include "spark/scene/foliage/GrassChunkScatterBuilder.hpp"

#include "spark/scene/foliage/GrassInstanceTintGenerator.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"

#include <cmath>
#include <cstdint>

namespace Spark {

float GrassChunkScatterBuilder::Hash01(const std::uint32_t seed) noexcept {
    std::uint32_t x = seed;
    x = (x ^ 61U) ^ (x >> 16U);
    x *= 9U;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2dU;
    x = x ^ (x >> 15U);
    return static_cast<float>(x & 0xFFFFU) / static_cast<float>(0xFFFFU);
}

bool GrassChunkScatterBuilder::TryPlaceBlade(
        const float worldX,
        const float worldZ,
        const std::uint32_t chunkSeed,
        const std::uint32_t placementSeed,
        const GrassChunkScatterSettings& settings,
        const GrassFieldBounds& bounds,
        const Vector3& fieldOriginWorld,
        const GrassTerrainSurfaceProbe& terrainProbe,
        Array<FoliageInstanceRecord>& outInstances) const {
    if (!bounds.ContainsWorldXZ(fieldOriginWorld, worldX, worldZ)) {
        return false;
    }

    const float placementRadius = settings.GetPlacementRadiusMeters();
    if (placementRadius > 0.0F) {
        const float localX = worldX - fieldOriginWorld.x;
        const float localZ = worldZ - fieldOriginWorld.z;
        const float distSq = localX * localX + localZ * localZ;
        const float radiusSq = placementRadius * placementRadius;
        if (distSq > radiusSq) {
            return false;
        }
        const float dist = std::sqrt(distSq);
        const float innerRadius = placementRadius * 0.76F;
        if (dist > innerRadius) {
            const float edgeT = (dist - innerRadius) / (placementRadius - innerRadius);
            const float keepChance = 1.0F - edgeT * edgeT;
            const float edgeRoll = Hash01(chunkSeed + placementSeed * 29U + 307U);
            if (edgeRoll > keepChance) {
                return false;
            }
        }
    }

    GrassTerrainSurfaceHit hit{};
    if (!terrainProbe.ProbeWorld(worldX, worldZ, hit)) {
        return false;
    }
    if (hit.SlopeAngleDegreesFromUp() > settings.GetMaxSlopeDegrees()) {
        return false;
    }

    const float r1 = Hash01(chunkSeed + placementSeed * 7U + 41U);
    const float r2 = Hash01(chunkSeed + placementSeed * 11U + 93U);
    const float r3 = Hash01(chunkSeed + placementSeed * 13U + 127U);
    const float r4 = Hash01(chunkSeed + placementSeed * 17U + 151U);
    const float r5 = Hash01(chunkSeed + placementSeed * 23U + 197U);

    const float yaw = r3 * TwoPi;
    const float widthScale = 0.48F + r1 * 1.05F;
    const float heightScale = 0.34F + r2 * 1.38F;
    const float depthScale = 0.42F + r4 * 1.08F;

    Matrix4 model = Matrix4::Translation({worldX, hit.GetWorldHeight(), worldZ});
    model = model * Matrix4::Rotation(Quaternion::FromAxisAngle(Vector3::UnitY, yaw));
    model = model * Matrix4::Scale({widthScale, heightScale, depthScale});

    FoliageInstanceRecord record{};
    record.SetModelMatrix(model);
    const float phaseSeed = Hash01(chunkSeed + placementSeed * 19U + 211U);
    record.SetWindPhase(phaseSeed * TwoPi);
    record.SetTint(GrassInstanceTintGenerator::BuildTint(settings.GetAlbedoTint(), r1, r2, r5));
    outInstances.PushBack(record);
    return true;
}

void GrassChunkScatterBuilder::ScatterChunk(
        const GrassChunkCoordinate& chunk,
        const GrassChunkScatterSettings& settings,
        const GrassFieldBounds& bounds,
        const Vector3& fieldOriginWorld,
        const GrassTerrainSurfaceProbe& terrainProbe,
        Array<FoliageInstanceRecord>& outInstances) const {
    const float chunkSize = settings.GetChunkSizeMeters();
    const float minX = chunk.WorldMinX(chunkSize);
    const float minZ = chunk.WorldMinZ(chunkSize);

    const std::uint32_t chunkInstanceCap = settings.GetMaxCachedInstancesPerChunk();
    const float chunkArea = chunkSize * chunkSize;
    const float capDensity = static_cast<float>(chunkInstanceCap) / chunkArea;
    float effectiveDensity = settings.GetDensityPerSquareMeter();
    if (capDensity < effectiveDensity) {
        effectiveDensity = capDensity;
    }

    const float spacing = 1.0F / std::sqrt(effectiveDensity);
    const int cells = static_cast<int>(std::floor(chunkSize / spacing));
    if (cells < 1) {
        return;
    }
    const float step = chunkSize / static_cast<float>(cells);
    const int samplesPerCell = settings.GetSamplesPerCell();

    const std::uint32_t chunkSeed =
            static_cast<std::uint32_t>(chunk.GetIndexX() * 73856093) ^
            static_cast<std::uint32_t>(chunk.GetIndexZ() * 19349663);

    std::uint32_t sampleIndex = 0U;
    for (int iz = 0; iz < cells; ++iz) {
        for (int ix = 0; ix < cells; ++ix) {
            if (outInstances.GetSize() >= chunkInstanceCap) {
                return;
            }

            const float r0 = Hash01(chunkSeed + sampleIndex * 3U + 17U);
            const float r1 = Hash01(chunkSeed + sampleIndex * 7U + 41U);
            const float r2 = Hash01(chunkSeed + sampleIndex * 11U + 93U);
            ++sampleIndex;

            const float jitterX = (r1 - 0.5F) * step * 0.98F;
            const float jitterZ = (r2 - 0.5F) * step * 0.98F;
            const float worldX = minX + (static_cast<float>(ix) + 0.5F) * step + jitterX;
            const float worldZ = minZ + (static_cast<float>(iz) + 0.5F) * step + jitterZ;

            TryPlaceBlade(
                    worldX,
                    worldZ,
                    chunkSeed,
                    sampleIndex * 2U,
                    settings,
                    bounds,
                    fieldOriginWorld,
                    terrainProbe,
                    outInstances);

            if (samplesPerCell >= 2 && outInstances.GetSize() < chunkInstanceCap) {
                const float subJitterX = (Hash01(chunkSeed + sampleIndex * 5U) - 0.5F) * step * 0.45F;
                const float subJitterZ = (Hash01(chunkSeed + sampleIndex * 9U) - 0.5F) * step * 0.45F;
                const float subX = worldX + step * 0.5F + subJitterX;
                const float subZ = worldZ + step * 0.5F + subJitterZ;
                if (r0 > 0.08F) {
                    TryPlaceBlade(
                            subX,
                            subZ,
                            chunkSeed,
                            sampleIndex * 2U + 1U,
                            settings,
                            bounds,
                            fieldOriginWorld,
                            terrainProbe,
                            outInstances);
                }
            }
        }
    }
}

}  // namespace Spark
