#include "spark/ecs/components/rendering/TerrainComponent.hpp"

#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/TerrainMeshGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Spark {

TerrainComponent::TerrainComponent(TerrainGeneratorSettings settings, Vector3 meshAlbedo)
        : settings(settings), meshAlbedo(meshAlbedo) {}

void TerrainComponent::OnAttach(GameObject& owner) {
    RegenerateMesh(owner);
}

void TerrainComponent::EnsureHeightBuffer(GameObject& /*owner*/) {
    const std::size_t need = TerrainMeshGenerator::HeightSampleCount(settings);
    if (heightSamples.GetSize() != need) {
        TerrainMeshGenerator::FillProceduralHeights(settings, heightSamples);
    }
}

void TerrainComponent::ResetHeightsToProcedural(GameObject& owner) {
    heightSamples.Clear();
    TerrainMeshGenerator::FillProceduralHeights(settings, heightSamples);
    RegenerateMesh(owner);
}

void TerrainComponent::RegenerateMesh(GameObject& owner) {
    EnsureHeightBuffer(owner);
    SharedPtr<Mesh> mesh = MakeShared<Mesh>(Utf8String("ProceduralTerrain"));
    TerrainMeshGenerator::BuildIntoFromHeights(*mesh, settings, heightSamples);

    MeshComponent* mc = owner.GetComponent<MeshComponent>();
    if (mc == nullptr) {
        owner.AddComponent<MeshComponent>(mesh, SceneMeshSlot::Custom, meshAlbedo);
    } else {
        mc->SetMesh(mesh);
        mc->SetAlbedo(meshAlbedo);
    }
}

bool TerrainComponent::RayTriangle(
        Vector3 ro, Vector3 rd, Vector3 v0, Vector3 v1, Vector3 v2, float& outT) noexcept {
    constexpr float kEps = 1.0e-6F;
    const Vector3 e1 = v1 - v0;
    const Vector3 e2 = v2 - v0;
    const Vector3 p = Vector3::Cross(rd, e2);
    const float det = Vector3::Dot(e1, p);
    if (std::fabs(det) < kEps) {
        return false;
    }
    const float invDet = 1.0F / det;
    const Vector3 tvec = ro - v0;
    const float u = Vector3::Dot(tvec, p) * invDet;
    if (u < 0.0F || u > 1.0F) {
        return false;
    }
    const Vector3 q = Vector3::Cross(tvec, e1);
    const float v = Vector3::Dot(rd, q) * invDet;
    if (v < 0.0F || u + v > 1.0F) {
        return false;
    }
    const float t = Vector3::Dot(e2, q) * invDet;
    if (t <= kEps) {
        return false;
    }
    outT = t;
    return true;
}

bool TerrainComponent::TryRaycastWorld(
        const GameObject& owner,
        Vector3 rayOriginWorld,
        Vector3 rayDirWorld,
        float maxDistance,
        Vector3& outHitWorld) const {
    if (heightSamples.IsEmpty()) {
        return false;
    }
    const std::int32_t nx = TerrainMeshGenerator::GridVertexCountX(settings);
    const std::int32_t nz = TerrainMeshGenerator::GridVertexCountZ(settings);
    const std::size_t need = TerrainMeshGenerator::HeightSampleCount(settings);
    if (heightSamples.GetSize() != need) {
        return false;
    }

    Vector3 rd = rayDirWorld;
    if (rd.LengthSquared() < Epsilon) {
        return false;
    }
    rd = rd.Normalized();

    const Matrix4 world = owner.GetWorldMatrix();
    Matrix4 inv{};
    if (!world.TryInvert(inv)) {
        return false;
    }
    const Vector3 roL = inv.TransformPoint(rayOriginWorld);
    const Vector3 rdL = inv.TransformVector(rd).Normalized();

    const float hx = settings.halfExtentX;
    const float hz = settings.halfExtentZ;

    auto vertexLocal = [&](std::int32_t ix, std::int32_t iz) -> Vector3 {
        const float tx = static_cast<float>(ix) / static_cast<float>(nx - 1);
        const float tz = static_cast<float>(iz) / static_cast<float>(nz - 1);
        const float x = -hx + tx * (2.0F * hx);
        const float z = -hz + tz * (2.0F * hz);
        const std::size_t idx = static_cast<std::size_t>(iz * nx + ix);
        return {x, heightSamples[idx], z};
    };

    float bestT = maxDistance;
    bool hit = false;
    for (std::int32_t iz = 0; iz < nz - 1; ++iz) {
        for (std::int32_t ix = 0; ix < nx - 1; ++ix) {
            const Vector3 v00 = vertexLocal(ix, iz);
            const Vector3 v10 = vertexLocal(ix + 1, iz);
            const Vector3 v01 = vertexLocal(ix, iz + 1);
            const Vector3 v11 = vertexLocal(ix + 1, iz + 1);
            float t = maxDistance;
            if (RayTriangle(roL, rdL, v00, v01, v10, t) && t < bestT) {
                bestT = t;
                hit = true;
            }
            if (RayTriangle(roL, rdL, v10, v01, v11, t) && t < bestT) {
                bestT = t;
                hit = true;
            }
        }
    }
    if (!hit) {
        return false;
    }
    const Vector3 hitLocal = roL + rdL * bestT;
    outHitWorld = world.TransformPoint(hitLocal);
    return true;
}

void TerrainComponent::ApplyHeightBrushLocal(GameObject& owner, Vector2 centerXZ, float radiusXZ, float deltaY) {
    if (radiusXZ <= 0.0F || heightSamples.IsEmpty()) {
        return;
    }
    const std::int32_t nx = TerrainMeshGenerator::GridVertexCountX(settings);
    const std::int32_t nz = TerrainMeshGenerator::GridVertexCountZ(settings);
    const float hx = settings.halfExtentX;
    const float hz = settings.halfExtentZ;
    bool changed = false;
    for (std::int32_t iz = 0; iz < nz; ++iz) {
        const float tz = static_cast<float>(iz) / static_cast<float>(nz - 1);
        const float z = -hz + tz * (2.0F * hz);
        for (std::int32_t ix = 0; ix < nx; ++ix) {
            const float tx = static_cast<float>(ix) / static_cast<float>(nx - 1);
            const float x = -hx + tx * (2.0F * hx);
            const float dx = x - centerXZ.x;
            const float dz = z - centerXZ.y;
            const float dist = std::sqrt(dx * dx + dz * dz);
            if (dist >= radiusXZ) {
                continue;
            }
            const float t = 1.0F - dist / radiusXZ;
            const float w = t * t * (3.0F - 2.0F * t);
            const std::size_t idx = static_cast<std::size_t>(iz * nx + ix);
            heightSamples[idx] += deltaY * w;
            changed = true;
        }
    }
    if (changed) {
        RegenerateMesh(owner);
    }
}

void TerrainComponent::ApplyHeightBrushWorld(GameObject& owner, Vector3 centerWorld, float radiusWorld, float deltaY) {
    if (radiusWorld <= 0.0F) {
        return;
    }
    const Matrix4 world = owner.GetWorldMatrix();
    Matrix4 inv{};
    if (!world.TryInvert(inv)) {
        return;
    }
    const Vector3 cL = inv.TransformPoint(centerWorld);
    const auto colLen = [](const Matrix4& m, int col) -> float {
        const int b = col * 4;
        const float x = m.m[b + 0];
        const float y = m.m[b + 1];
        const float z = m.m[b + 2];
        return std::sqrt(x * x + y * y + z * z);
    };
    const float sx = colLen(world, 0);
    const float sz = colLen(world, 2);
    const float horiz = (std::max)(sx, sz);
    const float radiusLocal = horiz > 1.0e-6F ? radiusWorld / horiz : radiusWorld;
    ApplyHeightBrushLocal(owner, {cL.x, cL.z}, radiusLocal, deltaY);
}

}  // namespace Spark
