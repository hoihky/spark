#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/mesh/MeshInstanceRecord.hpp"

namespace Spark {

/**
 * Opaque rigid mesh instancing on the main lit pass (props / crowds).
 * Use with <c>MeshComponent</c> + optional <c>MaterialComponent</c> on the same object.
 */
class RigidMeshInstancedBatchComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::RigidMeshInstancedBatch;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void ConfigureGrid(const int columns, const int rows, const float spacingMeters) noexcept;

    void AppendInstanceRecords(Array<MeshInstanceRecord>& outInstances, const Matrix4& ownerWorld);

private:
    void RebuildInstanceCache();

    bool enabled = true;
    int gridColumns = 8;
    int gridRows = 8;
    float gridSpacing = 1.0F;
    bool gridDirty = true;
    Array<Matrix4> cachedLocalModels{};
};

}  // namespace Spark