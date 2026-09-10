#include "spark/scene/water/WaterSurfaceMesh.hpp"

#include "spark/core/Utf8String.hpp"
#include "spark/scene/water/GerstnerWaveSurface.hpp"
#include "spark/scene/water/WaterWavePreset.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Spark {

WaterSurfaceMesh::WaterSurfaceMesh(WaterSurfaceMeshSettings settingsIn) : settings(settingsIn) {
    mesh = MakeShared<Mesh>(Utf8String("WaterSurface"));
}

void WaterSurfaceMesh::SetSettings(WaterSurfaceMeshSettings value) noexcept {
    settings = value;
    hasAnchor = false;
    restLocalPositions.Clear();
}

int WaterSurfaceMesh::ClampSubdivisionsPerAxis(int subdivisionsPerAxis) noexcept {
    return (std::max)(2, subdivisionsPerAxis);
}

std::size_t WaterSurfaceMesh::VertexCountForSubdivisions(int subdivisionsPerAxis) noexcept {
    const int subdiv = ClampSubdivisionsPerAxis(subdivisionsPerAxis);
    const int vertexCountPerAxis = subdiv + 1;
    return static_cast<std::size_t>(vertexCountPerAxis) * static_cast<std::size_t>(vertexCountPerAxis);
}

std::size_t WaterSurfaceMesh::IndexCountForSubdivisions(int subdivisionsPerAxis) noexcept {
    const int subdiv = ClampSubdivisionsPerAxis(subdivisionsPerAxis);
    return static_cast<std::size_t>(subdiv) * static_cast<std::size_t>(subdiv) * 6U;
}

void WaterSurfaceMesh::BuildFlatTileMesh(float tileHalfExtentX, float tileHalfExtentZ) {
    const int subdiv = ClampSubdivisionsPerAxis(settings.GetSubdivisionsPerAxis());
    const int vertexCountPerAxis = subdiv + 1;
    const float halfX = tileHalfExtentX;
    const float halfZ = tileHalfExtentZ;
    const float spanX = halfX * 2.0F;
    const float spanZ = halfZ * 2.0F;
    const float uvSpanX = spanX / ((settings.GetWorldUnitsPerTextureRepeat() > 0.0F)
                                           ? settings.GetWorldUnitsPerTextureRepeat()
                                           : (std::max)(spanX, 1.0e-3F));
    const float uvSpanZ = spanZ / ((settings.GetWorldUnitsPerTextureRepeat() > 0.0F)
                                           ? settings.GetWorldUnitsPerTextureRepeat()
                                           : (std::max)(spanZ, 1.0e-3F));

    mesh->Clear();
    mesh->GetVertices().Reserve(VertexCountForSubdivisions(subdiv));
    mesh->GetIndices().Reserve(IndexCountForSubdivisions(subdiv));

    for (int iz = 0; iz < vertexCountPerAxis; ++iz) {
        const float tz = static_cast<float>(iz) / static_cast<float>(subdiv);
        const float z = -halfZ + tz * spanZ;
        const float v = tz * uvSpanZ;
        for (int ix = 0; ix < vertexCountPerAxis; ++ix) {
            const float tx = static_cast<float>(ix) / static_cast<float>(subdiv);
            const float x = -halfX + tx * spanX;
            const float u = tx * uvSpanX;
            Mesh::Vertex vertex{};
            vertex.position = {x, 0.0F, z};
            vertex.normal = {0.0F, 1.0F, 0.0F};
            vertex.texCoord = {u, v};
            mesh->AddVertex(vertex);
        }
    }

    for (int iz = 0; iz < subdiv; ++iz) {
        for (int ix = 0; ix < subdiv; ++ix) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(iz * vertexCountPerAxis + ix);
            const std::uint32_t i1 = i0 + 1U;
            const std::uint32_t i2 = i0 + static_cast<std::uint32_t>(vertexCountPerAxis);
            const std::uint32_t i3 = i2 + 1U;
            // Match Mesh::CreateGroundPlane winding (+Y front face with Vulkan clip-Y / CLOCKWISE).
            mesh->AddTriangle(i0, i1, i2);
            mesh->AddTriangle(i1, i3, i2);
        }
    }

    restLocalPositions.Clear();
    restLocalPositions.Reserve(mesh->GetVertices().GetSize());
    for (std::size_t vi = 0; vi < mesh->GetVertices().GetSize(); ++vi) {
        restLocalPositions.PushBack(mesh->GetVertices()[vi].position);
    }
}

void WaterSurfaceMesh::RebuildTile(float tileHalfExtentX, float tileHalfExtentZ, Vector2 worldCenterXZ) {
    BuildFlatTileMesh(tileHalfExtentX, tileHalfExtentZ);
    anchorXZ = worldCenterXZ;
    hasAnchor = true;
    mesh->NotifyGeometryChanged();
}

void WaterSurfaceMesh::ApplyPreviewWaves(const float timeSeconds, const WaterWavePresetId preset) {
    if (!hasAnchor || restLocalPositions.IsEmpty() || !mesh) {
        return;
    }
    Array<Mesh::Vertex>& vertices = mesh->GetVertices();
    if (vertices.GetSize() != restLocalPositions.GetSize()) {
        return;
    }

    const GerstnerWaveSurface surface = WaterWavePreset(preset).ToSurface();
    for (std::size_t vi = 0; vi < vertices.GetSize(); ++vi) {
        const Vector3& rest = restLocalPositions[vi];
        const float worldX = anchorXZ.x + rest.x;
        const float worldZ = anchorXZ.y + rest.z;
        vertices[vi].position.y = surface.SampleHeight(worldX, worldZ, timeSeconds);
        vertices[vi].normal = surface.SampleNormal(worldX, worldZ, timeSeconds);
    }
    mesh->NotifyGeometryChanged();
}

bool WaterSurfaceMesh::ShouldRebuildForCamera(const Vector3& cameraWorld) const noexcept {
    if (!hasAnchor) {
        return true;
    }
    const float dx = cameraWorld.x - anchorXZ.x;
    const float dz = cameraWorld.z - anchorXZ.y;
    const float threshold = settings.GetRebuildMoveThreshold();
    if (threshold <= 0.0F) {
        return true;
    }
    return (dx * dx + dz * dz) > (threshold * threshold);
}

Vector2 WaterSurfaceMesh::ComputeSnappedAnchorXZ(const Vector3& cameraWorld) const noexcept {
    const float span = settings.GetTileHalfExtent() * 2.0F;
    if (span <= 1.0e-4F) {
        return {cameraWorld.x, cameraWorld.z};
    }
    const float anchorX = std::floor(cameraWorld.x / span) * span + span * 0.5F;
    const float anchorZ = std::floor(cameraWorld.z / span) * span + span * 0.5F;
    return {anchorX, anchorZ};
}

}  // namespace Spark
