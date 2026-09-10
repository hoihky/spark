#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/water/WaterBodyExtent.hpp"
#include "spark/scene/water/WaterBodyMode.hpp"
#include "spark/scene/water/WaterSurfaceMesh.hpp"
#include "spark/scene/water/WaterSurfaceMeshSettings.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

#include <optional>

namespace Spark {

class GameObject;
class GameWorld;
class MaterialComponent;

/**
 * ECS water volume: mode, level, extent, wave preset, and a `WaterSurfaceMesh` tile.
 * Attaches a `MeshComponent` on `OnAttach` for the standard lit scene path (dedicated water pass is W0-03+).
 */
class WaterBodyComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::WaterBody;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    WaterBodyComponent() noexcept = default;

    WaterBodyComponent(
            WaterBodyMode modeIn,
            float waterLevelYIn,
            WaterBodyExtent extentIn,
            WaterWavePresetId wavePresetIn,
            WaterSurfaceMeshSettings meshSettingsIn = {},
            Vector3 meshAlbedoIn = Vector3{0.08F, 0.35F, 0.55F});

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    [[nodiscard]] WaterBodyMode GetMode() const noexcept { return mode; }
    [[nodiscard]] float GetWaterLevelY() const noexcept { return waterLevelY; }
    [[nodiscard]] const WaterBodyExtent& GetExtent() const noexcept { return extent; }
    [[nodiscard]] WaterWavePresetId GetWavePresetId() const noexcept { return wavePresetId; }
    [[nodiscard]] float GetWaveTimeSeconds() const noexcept { return waveTimeSeconds; }
    [[nodiscard]] float GetWaveTimeScale() const noexcept { return waveTimeScale; }
    [[nodiscard]] WaterWaveSettings GetResolvedWaveSettings() const noexcept;
    [[nodiscard]] const WaterSurfaceMeshSettings& GetSurfaceMeshSettings() const noexcept {
        return surfaceMesh.GetSettings();
    }
    [[nodiscard]] const WaterSurfaceMesh& GetSurfaceMesh() const noexcept { return surfaceMesh; }
    [[nodiscard]] const Vector3& GetMeshAlbedo() const noexcept { return meshAlbedo; }
    /** Combines mesh albedo with optional material tint for the water shader pass. */
    [[nodiscard]] Vector3 GetResolvedSurfaceAlbedo(const MaterialComponent* material) const noexcept;

    void SetMode(WaterBodyMode value) noexcept;
    void SetWaterLevelY(float value) noexcept;
    void SetExtent(const WaterBodyExtent& value) noexcept;
    void SetWavePresetId(WaterWavePresetId value) noexcept;
    void SetSurfaceMeshSettings(WaterSurfaceMeshSettings value) noexcept;
    void SetMeshAlbedo(const Vector3& value) noexcept;
    void SetWaveTimeScale(float value) noexcept { waveTimeScale = value; }
    void SetWaveAmplitudeScale(float value) noexcept { waveAmplitudeScale = value; }
    void SetWaveSpeedScale(float value) noexcept { waveSpeedScale = value; }
    [[nodiscard]] float GetWaveAmplitudeScale() const noexcept { return waveAmplitudeScale; }
    [[nodiscard]] float GetWaveSpeedScale() const noexcept { return waveSpeedScale; }

    /** Overrides ECS camera lookup for infinite-ocean clipmap recentring (e.g. fly-camera demos). */
    void SetClipmapCameraWorld(const Vector3& worldPosition) noexcept;
    void ClearClipmapCameraOverride() noexcept;

    /** Rebuilds the surface mesh and syncs `MeshComponent` (call after changing mode/extent/settings). */
    void RegenerateSurface(GameObject& owner);

    /** Recenters infinite-ocean tiles from the main camera when the clipmap threshold is exceeded. */
    void UpdateInfiniteOceanClipmap(GameObject& owner, const Vector3& cameraWorld);

private:
    [[nodiscard]] float ResolveTileHalfExtentX() const noexcept;
    [[nodiscard]] float ResolveTileHalfExtentZ() const noexcept;
    [[nodiscard]] Vector2 ResolveFiniteCenterXZ(const GameObject& owner) const noexcept;

    void SyncTransformToSurface(GameObject& owner, Vector2 centerXZ) const;
    void ApplyMeshToOwner(GameObject& owner);

    WaterBodyMode mode = WaterBodyMode::InfiniteOcean;
    float waterLevelY = 0.0F;
    WaterBodyExtent extent = WaterBodyExtent::MakeInfinitePlaceholder();
    WaterWavePresetId wavePresetId = WaterWavePresetId::CalmLake;
    WaterSurfaceMesh surfaceMesh{};
    Vector3 meshAlbedo{0.08F, 0.35F, 0.55F};
    float waveTimeSeconds = 0.0F;
    float waveTimeScale = 1.0F;
    float waveAmplitudeScale = 1.0F;
    float waveSpeedScale = 1.0F;
    bool surfaceDirty = true;
    std::optional<Vector3> clipmapCameraOverride{};

    [[nodiscard]] bool TryResolveClipmapCameraWorld(const GameWorld& world, Vector3& outWorld) const noexcept;
};

}  // namespace Spark
