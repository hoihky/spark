#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * GPU-instanced foliage field on a transform (F1 demo / precursor to F2 grass chunks).
 * Instances are regenerated when grid settings change.
 */
class FoliageInstancedMeshComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::FoliageInstancedMesh;

    FoliageInstancedMeshComponent() = default;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetBladeMesh(const SharedPtr<Mesh>& mesh) noexcept;
    [[nodiscard]] const SharedPtr<Mesh>& GetBladeMesh() const noexcept { return bladeMesh; }

    void SetAlbedoTexture(const SharedPtr<Texture2D>& texture) noexcept { albedoTexture = texture; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetAlbedoTexture() const noexcept { return albedoTexture; }

    SPARK_SCRIPT_BIND(set_albedo_tint)
    void SetAlbedoTint(const Vector3& rgb) noexcept { albedoTint = rgb; }
    SPARK_SCRIPT_BIND(get_albedo_tint)
    [[nodiscard]] Vector3 GetAlbedoTint() const noexcept { return albedoTint; }

    SPARK_SCRIPT_BIND(set_alpha_cutoff)
    void SetAlphaCutoff(float value) noexcept { alphaCutoff = value; }
    SPARK_SCRIPT_BIND(get_alpha_cutoff)
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }

    SPARK_SCRIPT_BIND(set_wind_bend_scale)
    void SetWindBendScale(float scale) noexcept { windBendScale = scale; }
    SPARK_SCRIPT_BIND(get_wind_bend_scale)
    [[nodiscard]] float GetWindBendScale() const noexcept { return windBendScale; }

    SPARK_SCRIPT_BIND(configure_grid)
    void ConfigureGrid(int columns, int rows, float spacingMeters) noexcept;

    /**
     * Fills a square patch centered on the owner, inset by <c>edgeMarginMeters</c> from
     * <c>±halfExtentMeters</c> on XZ (matches a ground plane from <c>Mesh::CreateGroundPlane</c>).
     */
    SPARK_SCRIPT_BIND(configure_grid_within_square)
    void ConfigureGridWithinSquare(float halfExtentMeters, float spacingMeters, float edgeMarginMeters = 0.4F) noexcept;

    void AppendInstanceRecords(Array<FoliageInstanceRecord>& outInstances, const Matrix4& ownerWorld) const;

private:
    void RebuildInstanceCache();

    bool enabled = true;
    SharedPtr<Mesh> bladeMesh{};
    SharedPtr<Texture2D> albedoTexture{};
    Vector3 albedoTint{0.55F, 0.95F, 0.42F};
    float alphaCutoff = 0.38F;
    float windBendScale = 0.28F;

    int gridColumns = 24;
    int gridRows = 24;
    float gridSpacing = 0.35F;
    float coverageHalfExtent = 0.0F;
    float edgeMarginMeters = 0.4F;
    bool useSquareBounds = false;
    bool gridDirty = true;

    Array<FoliageInstanceRecord> cachedInstances{};
};

}  // namespace Spark
