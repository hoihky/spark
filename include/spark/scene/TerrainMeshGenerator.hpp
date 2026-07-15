#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/TerrainGeneratorSettings.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class Mesh;

/**
 * Builds an indexed XZ heightfield mesh (normals + UVs) from procedural noise or explicit height samples.
 */
class TerrainMeshGenerator {
public:
    [[nodiscard]] static std::int32_t GridVertexCountX(const TerrainGeneratorSettings& settings) noexcept;
    [[nodiscard]] static std::int32_t GridVertexCountZ(const TerrainGeneratorSettings& settings) noexcept;
    [[nodiscard]] static std::size_t HeightSampleCount(const TerrainGeneratorSettings& settings) noexcept;

    /** Resizes @p outHeights to nx*nz and fills Y heights from fBM (same formula as legacy BuildInto). */
    static void FillProceduralHeights(const TerrainGeneratorSettings& settings, Array<float>& outHeights);

    /** Expects @p heights size == HeightSampleCount(settings). */
    static void BuildIntoFromHeights(Mesh& mesh, const TerrainGeneratorSettings& settings, const Array<float>& heights);

    static void BuildInto(Mesh& mesh, const TerrainGeneratorSettings& settings);
};

}  // namespace Spark
