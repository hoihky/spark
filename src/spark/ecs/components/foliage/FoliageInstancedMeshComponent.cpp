#include "spark/ecs/components/foliage/FoliageInstancedMeshComponent.hpp"

#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

float Hash01(std::uint32_t seed) noexcept {
    seed = (seed ^ 61U) ^ (seed >> 16U);
    seed *= 9U;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2dU;
    seed = seed ^ (seed >> 15U);
    return static_cast<float>(seed & 0xFFFFU) / static_cast<float>(0xFFFFU);
}

}  // namespace

void FoliageInstancedMeshComponent::OnAttach(GameObject& owner) {
    (void)owner;
    gridDirty = true;
}

void FoliageInstancedMeshComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& context) {
    (void)timing;
    (void)owner;
    (void)context;
    if (gridDirty) {
        RebuildInstanceCache();
    }
}

void FoliageInstancedMeshComponent::SetBladeMesh(const SharedPtr<Mesh>& mesh) noexcept {
    bladeMesh = mesh;
    gridDirty = true;
}

void FoliageInstancedMeshComponent::ConfigureGrid(
        const int columns,
        const int rows,
        const float spacingMeters) noexcept {
    useSquareBounds = false;
    gridColumns = columns > 0 ? columns : 1;
    gridRows = rows > 0 ? rows : 1;
    gridSpacing = spacingMeters > 0.05F ? spacingMeters : 0.35F;
    gridDirty = true;
}

void FoliageInstancedMeshComponent::ConfigureGridWithinSquare(
        const float halfExtentMeters,
        const float spacingMeters,
        const float edgeMarginMetersIn) noexcept {
    useSquareBounds = true;
    coverageHalfExtent = halfExtentMeters > 0.5F ? halfExtentMeters : 0.5F;
    edgeMarginMeters = edgeMarginMetersIn >= 0.0F ? edgeMarginMetersIn : 0.4F;
    gridSpacing = spacingMeters > 0.05F ? spacingMeters : 0.35F;
    const float innerSpan = 2.0F * (coverageHalfExtent - edgeMarginMeters);
    int cells = 1;
    if (innerSpan > gridSpacing) {
        cells = static_cast<int>(std::floor(innerSpan / gridSpacing)) + 1;
    }
    if (cells < 1) {
        cells = 1;
    }
    gridColumns = cells;
    gridRows = cells;
    gridDirty = true;
}

void FoliageInstancedMeshComponent::RebuildInstanceCache() {
    cachedInstances.Clear();
    gridDirty = false;
    if (!bladeMesh) {
        return;
    }
    const float minBound = useSquareBounds ? (-coverageHalfExtent + edgeMarginMeters) : 0.0F;
    const float maxBound = useSquareBounds ? (coverageHalfExtent - edgeMarginMeters) : 0.0F;
    const float originX = useSquareBounds ? minBound
                                          : -static_cast<float>(gridColumns - 1) * gridSpacing * 0.5F;
    const float originZ = useSquareBounds ? minBound
                                          : -static_cast<float>(gridRows - 1) * gridSpacing * 0.5F;
    const float stepX = useSquareBounds && gridColumns > 1
            ? (maxBound - minBound) / static_cast<float>(gridColumns - 1)
            : gridSpacing;
    const float stepZ = useSquareBounds && gridRows > 1
            ? (maxBound - minBound) / static_cast<float>(gridRows - 1)
            : gridSpacing;
    std::uint32_t index = 0U;
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridColumns; ++col) {
            const float r0 = Hash01(index * 3U + 17U);
            const float r1 = Hash01(index * 7U + 41U);
            const float r2 = Hash01(index * 11U + 93U);
            const float r3 = Hash01(index * 13U + 127U);
            if (r0 < 0.08F) {
                ++index;
                continue;
            }

            const float jitterScale = useSquareBounds ? 0.35F : 0.82F;
            const float jitterX = (r1 - 0.5F) * stepX * jitterScale;
            const float jitterZ = (r2 - 0.5F) * stepZ * jitterScale;
            float x = originX + static_cast<float>(col) * stepX + jitterX;
            float z = originZ + static_cast<float>(row) * stepZ + jitterZ;
            if (useSquareBounds) {
                x = std::clamp(x, minBound, maxBound);
                z = std::clamp(z, minBound, maxBound);
            }
            const float yaw = r3 * TwoPi;
            const float scale = 0.78F + r1 * 0.42F;

            Matrix4 model = Matrix4::Translation({x, 0.0F, z});
            model = model * Matrix4::Rotation(Quaternion::FromAxisAngle(Vector3::UnitY, yaw));
            model = model * Matrix4::Scale({scale, scale * (0.88F + r2 * 0.28F), scale});

            FoliageInstanceRecord record{};
            record.SetModelMatrix(model);
            record.SetWindPhase(r2 * TwoPi);
            const float tintShift = 0.88F + r3 * 0.18F;
            record.SetTint(Vector4{albedoTint.x * tintShift, albedoTint.y * tintShift, albedoTint.z * tintShift, 1.0F});
            cachedInstances.PushBack(record);
            ++index;
        }
    }
}

void FoliageInstancedMeshComponent::AppendInstanceRecords(
        Array<FoliageInstanceRecord>& outInstances,
        const Matrix4& ownerWorld) const {
    if (!enabled || cachedInstances.IsEmpty()) {
        return;
    }
    for (std::size_t i = 0; i < cachedInstances.GetSize(); ++i) {
        FoliageInstanceRecord worldRecord = cachedInstances[i];
        worldRecord.SetModelMatrix(ownerWorld * worldRecord.GetModelMatrix());
        outInstances.PushBack(worldRecord);
    }
}

}  // namespace Spark
