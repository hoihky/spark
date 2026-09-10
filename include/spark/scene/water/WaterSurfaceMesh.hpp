#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/water/WaterSurfaceMeshSettings.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"

namespace Spark {

/** Builds and recenters a subdivided horizontal water tile (XZ plane, +Y normal). */
class WaterSurfaceMesh {
public:
    explicit WaterSurfaceMesh(WaterSurfaceMeshSettings settings = {});

    [[nodiscard]] const WaterSurfaceMeshSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(WaterSurfaceMeshSettings value) noexcept;

    [[nodiscard]] const SharedPtr<Mesh>& GetMesh() const noexcept { return mesh; }

    /** World-space anchor (XZ center of the current tile). Y is not authoritative. */
    [[nodiscard]] Vector2 GetAnchorXZ() const noexcept { return anchorXZ; }

    [[nodiscard]] bool HasAnchor() const noexcept { return hasAnchor; }

    /**
     * @param tileHalfExtentX Half-extent along local X.
     * @param tileHalfExtentZ Half-extent along local Z.
     * @param worldCenterXZ World XZ center for the tile anchor.
     */
    void RebuildTile(float tileHalfExtentX, float tileHalfExtentZ, Vector2 worldCenterXZ);

    /**
     * Infinite-ocean clipmap: true when the camera moved beyond `GetRebuildMoveThreshold` from the anchor.
     */
    [[nodiscard]] bool ShouldRebuildForCamera(const Vector3& cameraWorld) const noexcept;

    /**
     * Snaps a camera position to the center of the clipmap tile grid.
     * Tile span = 2 × tile half extent from settings.
     */
    [[nodiscard]] Vector2 ComputeSnappedAnchorXZ(const Vector3& cameraWorld) const noexcept;

    /** CPU Gerstner preview (W0); replaced by GPU displacement in W1. */
    void ApplyPreviewWaves(float timeSeconds, WaterWavePresetId preset);

    [[nodiscard]] static int ClampSubdivisionsPerAxis(int subdivisionsPerAxis) noexcept;
    [[nodiscard]] static std::size_t VertexCountForSubdivisions(int subdivisionsPerAxis) noexcept;
    [[nodiscard]] static std::size_t IndexCountForSubdivisions(int subdivisionsPerAxis) noexcept;

private:
    void BuildFlatTileMesh(float tileHalfExtentX, float tileHalfExtentZ);

    WaterSurfaceMeshSettings settings{};
    SharedPtr<Mesh> mesh = MakeShared<Mesh>(Utf8String("WaterSurface"));
    Array<Vector3> restLocalPositions{};
    Vector2 anchorXZ{};
    bool hasAnchor = false;
};

}  // namespace Spark
