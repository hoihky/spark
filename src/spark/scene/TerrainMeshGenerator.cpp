#include "spark/scene/TerrainMeshGenerator.hpp"

#include "spark/math/Vector3.hpp"
#include "spark/scene/Mesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Spark {

namespace {

[[nodiscard]] std::uint32_t HashU32(std::uint32_t x) noexcept {
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    return x;
}

[[nodiscard]] float HashToUnit(std::int32_t ix, std::int32_t iz, std::uint32_t seed) noexcept {
    std::uint32_t h = HashU32(static_cast<std::uint32_t>(ix) * 0x9E3779B1u ^ static_cast<std::uint32_t>(iz) * 0x85EBCA6Bu ^ seed);
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0x1000000u);
}

[[nodiscard]] float SmoothNoise(float x, float z, std::uint32_t seed) noexcept {
    const int ix = static_cast<int>(std::floor(x));
    const int iz = static_cast<int>(std::floor(z));
    const float fx = x - static_cast<float>(ix);
    const float fz = z - static_cast<float>(iz);
    const float u = fx * fx * (3.0F - 2.0F * fx);
    const float v = fz * fz * (3.0F - 2.0F * fz);
    const float a = HashToUnit(ix, iz, seed);
    const float b = HashToUnit(ix + 1, iz, seed);
    const float c = HashToUnit(ix, iz + 1, seed);
    const float d = HashToUnit(ix + 1, iz + 1, seed);
    const float ab = a + (b - a) * u;
    const float cd = c + (d - c) * u;
    return ab + (cd - ab) * v;
}

[[nodiscard]] float Fbm(float x, float z, const TerrainGeneratorSettings& s) noexcept {
    float sum = 0.0F;
    float amp = 1.0F;
    float freq = 1.0F;
    float norm = 0.0F;
    for (std::int32_t i = 0; i < s.octaves; ++i) {
        sum += amp * SmoothNoise(x * freq, z * freq, s.seed + static_cast<std::uint32_t>(i) * 1315423911u);
        norm += amp;
        amp *= s.persistence;
        freq *= s.lacunarity;
    }
    return norm > 1.0e-6F ? sum / norm : 0.0F;
}

}  // namespace

std::int32_t TerrainMeshGenerator::GridVertexCountX(const TerrainGeneratorSettings& settings) noexcept {
    return (std::max)(std::int32_t{2}, settings.subdivX) + 1;
}

std::int32_t TerrainMeshGenerator::GridVertexCountZ(const TerrainGeneratorSettings& settings) noexcept {
    return (std::max)(std::int32_t{2}, settings.subdivZ) + 1;
}

std::size_t TerrainMeshGenerator::HeightSampleCount(const TerrainGeneratorSettings& settings) noexcept {
    const std::int32_t nx = GridVertexCountX(settings);
    const std::int32_t nz = GridVertexCountZ(settings);
    return static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz);
}

void TerrainMeshGenerator::FillProceduralHeights(const TerrainGeneratorSettings& settings, Array<float>& outHeights) {
    const std::int32_t nx = GridVertexCountX(settings);
    const std::int32_t nz = GridVertexCountZ(settings);
    const std::size_t need = static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz);
    outHeights.Clear();
    outHeights.Reserve(need);
    const float hx = settings.halfExtentX;
    const float hz = settings.halfExtentZ;
    for (std::int32_t iz = 0; iz < nz; ++iz) {
        const float tz = static_cast<float>(iz) / static_cast<float>(nz - 1);
        const float z = -hz + tz * (2.0F * hz);
        for (std::int32_t ix = 0; ix < nx; ++ix) {
            const float tx = static_cast<float>(ix) / static_cast<float>(nx - 1);
            const float x = -hx + tx * (2.0F * hx);
            const float nxw = x * settings.noiseScale;
            const float nzw = z * settings.noiseScale;
            const float h = (Fbm(nxw, nzw, settings) * 2.0F - 1.0F) * settings.heightScale;
            outHeights.PushBack(h);
        }
    }
}

void TerrainMeshGenerator::BuildIntoFromHeights(
        Mesh& mesh, const TerrainGeneratorSettings& settings, const Array<float>& heights) {
    mesh.Clear();
    const std::int32_t nx = GridVertexCountX(settings);
    const std::int32_t nz = GridVertexCountZ(settings);
    const std::size_t need = static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz);
    if (heights.GetSize() != need) {
        return;
    }
    const float hx = settings.halfExtentX;
    const float hz = settings.halfExtentZ;

    mesh.GetVertices().Reserve(need);
    for (std::int32_t iz = 0; iz < nz; ++iz) {
        const float tz = static_cast<float>(iz) / static_cast<float>(nz - 1);
        const float z = -hz + tz * (2.0F * hz);
        for (std::int32_t ix = 0; ix < nx; ++ix) {
            const float tx = static_cast<float>(ix) / static_cast<float>(nx - 1);
            const float x = -hx + tx * (2.0F * hx);
            const std::size_t idx = static_cast<std::size_t>(iz * nx + ix);
            Mesh::Vertex v{};
            v.position = {x, heights[idx], z};
            const float uvRepeat = (std::max)(settings.worldUnitsPerTextureRepeat, 1.0e-3F);
            v.texCoord = {x / uvRepeat, z / uvRepeat};
            mesh.AddVertex(v);
        }
    }

    auto& idxBuf = mesh.GetIndices();
    idxBuf.Reserve(static_cast<std::size_t>(nx - 1) * static_cast<std::size_t>(nz - 1) * 6U);
    for (std::int32_t iz = 0; iz < nz - 1; ++iz) {
        for (std::int32_t ix = 0; ix < nx - 1; ++ix) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(iz * nx + ix);
            const std::uint32_t i1 = i0 + 1U;
            const std::uint32_t i2 = static_cast<std::uint32_t>((iz + 1) * nx + ix);
            const std::uint32_t i3 = i2 + 1U;
            // Winding matches unit cube / Vulkan PerspectiveVulkan back-face culling (not glTF CCW).
            mesh.AddTriangle(i0, i1, i2);
            mesh.AddTriangle(i1, i3, i2);
        }
    }

    auto& verts = mesh.GetVertices();
    for (std::int32_t iz = 0; iz < nz; ++iz) {
        for (std::int32_t ix = 0; ix < nx; ++ix) {
            const std::size_t i = static_cast<std::size_t>(iz * nx + ix);
            const std::int32_t ixL = (std::max)(ix - 1, 0);
            const std::int32_t ixR = (std::min)(ix + 1, nx - 1);
            const std::int32_t izD = (std::max)(iz - 1, 0);
            const std::int32_t izU = (std::min)(iz + 1, nz - 1);
            const Vector3& L = verts[static_cast<std::size_t>(iz * nx + ixL)].position;
            const Vector3& R = verts[static_cast<std::size_t>(iz * nx + ixR)].position;
            const Vector3& D = verts[static_cast<std::size_t>(izD * nx + ix)].position;
            const Vector3& U = verts[static_cast<std::size_t>(izU * nx + ix)].position;
            Vector3 t{R.x - L.x, R.y - L.y, R.z - L.z};
            Vector3 b{U.x - D.x, U.y - D.y, U.z - D.z};
            Vector3 n = Vector3::Cross(b, t);
            if (n.LengthSquared() > 1.0e-12F) {
                n = n.Normalized();
            } else {
                n = Vector3::UnitY;
            }
            verts[i].normal = n;
        }
    }
}

void TerrainMeshGenerator::BuildInto(Mesh& mesh, const TerrainGeneratorSettings& settings) {
    Array<float> h{};
    FillProceduralHeights(settings, h);
    BuildIntoFromHeights(mesh, settings, h);
}

}  // namespace Spark
