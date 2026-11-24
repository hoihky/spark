#include "spark/ecs/components/rendering/RigidMeshInstancedBatchComponent.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"

#include <algorithm>

namespace Spark {

void RigidMeshInstancedBatchComponent::ConfigureGrid(
        const int columns,
        const int rows,
        const float spacingMeters) noexcept {
    gridColumns = std::max(columns, 1);
    gridRows = std::max(rows, 1);
    gridSpacing = std::max(spacingMeters, 0.05F);
    gridDirty = true;
}

void RigidMeshInstancedBatchComponent::RebuildInstanceCache() {
    cachedLocalModels.Clear();
    if (gridColumns <= 0 || gridRows <= 0) {
        gridDirty = false;
        return;
    }
    const float startX = -0.5F * static_cast<float>(gridColumns - 1) * gridSpacing;
    const float startZ = -0.5F * static_cast<float>(gridRows - 1) * gridSpacing;
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridColumns; ++col) {
            const Vector3 t{startX + static_cast<float>(col) * gridSpacing, 0.0F,
                            startZ + static_cast<float>(row) * gridSpacing};
            cachedLocalModels.PushBack(Matrix4::Translation(t));
        }
    }
    gridDirty = false;
}

void RigidMeshInstancedBatchComponent::AppendInstanceRecords(
        Array<MeshInstanceRecord>& outInstances,
        const Matrix4& ownerWorld) {
    if (!enabled) {
        return;
    }
    if (gridDirty) {
        RebuildInstanceCache();
    }
    for (std::size_t i = 0; i < cachedLocalModels.GetSize(); ++i) {
        if (outInstances.GetSize() >= SceneRenderParams::MaxRigidMeshInstances) {
            return;
        }
        MeshInstanceRecord record{};
        record.SetModelMatrix(ownerWorld * cachedLocalModels[i]);
        outInstances.PushBack(record);
    }
}

}  // namespace Spark