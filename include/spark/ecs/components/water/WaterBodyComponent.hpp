#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/water/WaterBodyExtent.hpp"
#include "spark/scene/water/WaterScreenSpaceReflectionSettings.hpp"
#include "spark/scene/water/WaterBodyMode.hpp"
#include "spark/scene/water/WaterSurfaceMesh.hpp"
#include "spark/scene/water/WaterSurfaceMeshSettings.hpp"
#include "spark/scene/water/WaterWavePresetId.hpp"
#include "spark/core/Optional.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_mode)
    [[nodiscard]] WaterBodyMode GetMode() const noexcept { return mode; }
    SPARK_SCRIPT_BIND(get_water_level_y)
    [[nodiscard]] float GetWaterLevelY() const noexcept { return waterLevelY; }
    SPARK_SCRIPT_BIND(get_extent)
    [[nodiscard]] const WaterBodyExtent& GetExtent() const noexcept { return extent; }
    SPARK_SCRIPT_BIND(get_wave_preset_id)
    [[nodiscard]] WaterWavePresetId GetWavePresetId() const noexcept { return wavePresetId; }
    SPARK_SCRIPT_BIND(get_wave_time_seconds)
    [[nodiscard]] float GetWaveTimeSeconds() const noexcept { return waveTimeSeconds; }
    SPARK_SCRIPT_BIND(get_wave_time_scale)
    [[nodiscard]] float GetWaveTimeScale() const noexcept { return waveTimeScale; }
    SPARK_SCRIPT_BIND(get_resolved_wave_settings)
    [[nodiscard]] WaterWaveSettings GetResolvedWaveSettings() const noexcept;
    [[nodiscard]] const WaterSurfaceMeshSettings& GetSurfaceMeshSettings() const noexcept {
        return surfaceMesh.GetSettings();
    }
    SPARK_SCRIPT_BIND(get_surface_mesh)
    [[nodiscard]] const WaterSurfaceMesh& GetSurfaceMesh() const noexcept { return surfaceMesh; }
    SPARK_SCRIPT_BIND(get_surface_tile_anchor_x_z)
    [[nodiscard]] Vector2 GetSurfaceTileAnchorXZ() const noexcept { return surfaceMesh.GetAnchorXZ(); }
    SPARK_SCRIPT_BIND(get_mesh_albedo)
    [[nodiscard]] const Vector3& GetMeshAlbedo() const noexcept { return meshAlbedo; }
    SPARK_SCRIPT_BIND(get_deep_color)
    [[nodiscard]] const Vector3& GetDeepColor() const noexcept { return deepColor; }
    SPARK_SCRIPT_BIND(get_absorption)
    [[nodiscard]] float GetAbsorption() const noexcept { return absorption; }
    SPARK_SCRIPT_BIND(get_foam_strength)
    [[nodiscard]] float GetFoamStrength() const noexcept { return foamStrength; }
    SPARK_SCRIPT_BIND(get_detail_normal_strength)
    [[nodiscard]] float GetDetailNormalStrength() const noexcept { return detailNormalStrength; }
    SPARK_SCRIPT_BIND(get_shoreline_foam_strength)
    [[nodiscard]] float GetShorelineFoamStrength() const noexcept { return shorelineFoamStrength; }
    SPARK_SCRIPT_BIND(get_shoreline_foam_max_depth)
    [[nodiscard]] float GetShorelineFoamMaxDepth() const noexcept { return shorelineFoamMaxDepth; }
    SPARK_SCRIPT_BIND(get_ssr_settings)
    [[nodiscard]] const WaterScreenSpaceReflectionSettings& GetSsrSettings() const noexcept { return ssrSettings; }
    /** Combines mesh albedo with optional material tint for the water shader pass. */
    SPARK_SCRIPT_BIND(get_resolved_surface_albedo)
    [[nodiscard]] Vector3 GetResolvedSurfaceAlbedo(const MaterialComponent* material) const noexcept;
    /** Combines deep color with optional material tint for depth absorption. */
    SPARK_SCRIPT_BIND(get_resolved_deep_color)
    [[nodiscard]] Vector3 GetResolvedDeepColor(const MaterialComponent* material) const noexcept;

    SPARK_SCRIPT_BIND(set_mode)
    void SetMode(WaterBodyMode value) noexcept;
    SPARK_SCRIPT_BIND(set_water_level_y)
    void SetWaterLevelY(float value) noexcept;
    SPARK_SCRIPT_BIND(set_extent)
    void SetExtent(const WaterBodyExtent& value) noexcept;
    SPARK_SCRIPT_BIND(set_wave_preset_id)
    void SetWavePresetId(WaterWavePresetId value) noexcept;
    SPARK_SCRIPT_BIND(set_surface_mesh_settings)
    void SetSurfaceMeshSettings(WaterSurfaceMeshSettings value) noexcept;
    SPARK_SCRIPT_BIND(set_mesh_albedo)
    void SetMeshAlbedo(const Vector3& value) noexcept;
    SPARK_SCRIPT_BIND(set_deep_color)
    void SetDeepColor(const Vector3& value) noexcept;
    SPARK_SCRIPT_BIND(set_absorption)
    void SetAbsorption(float value) noexcept;
    SPARK_SCRIPT_BIND(set_foam_strength)
    void SetFoamStrength(float value) noexcept;
    SPARK_SCRIPT_BIND(set_detail_normal_strength)
    void SetDetailNormalStrength(float value) noexcept;
    SPARK_SCRIPT_BIND(set_shoreline_foam_strength)
    void SetShorelineFoamStrength(float value) noexcept;
    SPARK_SCRIPT_BIND(set_shoreline_foam_max_depth)
    void SetShorelineFoamMaxDepth(float value) noexcept;
    SPARK_SCRIPT_BIND(set_ssr_settings)
    void SetSsrSettings(const WaterScreenSpaceReflectionSettings& value) noexcept { ssrSettings = value; }
    SPARK_SCRIPT_BIND(set_ssr_enabled)
    void SetSsrEnabled(const bool enabled) noexcept { ssrSettings.SetEnabled(enabled); }
    SPARK_SCRIPT_BIND(set_wave_time_scale)
    void SetWaveTimeScale(float value) noexcept { waveTimeScale = value; }
    SPARK_SCRIPT_BIND(set_wave_amplitude_scale)
    void SetWaveAmplitudeScale(float value) noexcept { waveAmplitudeScale = value; }
    SPARK_SCRIPT_BIND(set_wave_speed_scale)
    void SetWaveSpeedScale(float value) noexcept { waveSpeedScale = value; }
    SPARK_SCRIPT_BIND(get_wave_amplitude_scale)
    [[nodiscard]] float GetWaveAmplitudeScale() const noexcept { return waveAmplitudeScale; }
    SPARK_SCRIPT_BIND(get_wave_speed_scale)
    [[nodiscard]] float GetWaveSpeedScale() const noexcept { return waveSpeedScale; }
    /** World XZ travel direction for the dominant swell (wave 0); optional preset rotation. */
    SPARK_SCRIPT_BIND(set_swell_travel_direction_world)
    void SetSwellTravelDirectionWorld(Vector2 directionWorldXZ) noexcept;
    SPARK_SCRIPT_BIND(clear_swell_travel_direction_override)
    void ClearSwellTravelDirectionOverride() noexcept;
    [[nodiscard]] bool HasSwellTravelDirectionOverride() const noexcept {
        return swellTravelDirectionOverride.HasValue();
    }

    /** Overrides ECS camera lookup for infinite-ocean clipmap recentring (e.g. fly-camera demos). */
    SPARK_SCRIPT_BIND(set_clipmap_camera_world)
    void SetClipmapCameraWorld(const Vector3& worldPosition) noexcept;
    SPARK_SCRIPT_BIND(clear_clipmap_camera_override)
    void ClearClipmapCameraOverride() noexcept;

    /** Rebuilds the surface mesh and syncs `MeshComponent` (call after changing mode/extent/settings). */
    SPARK_SCRIPT_BIND(regenerate_surface)
    void RegenerateSurface(GameObject& owner);

    /** Recenters infinite-ocean tiles from the main camera when the clipmap threshold is exceeded. */
    SPARK_SCRIPT_BIND(update_infinite_ocean_clipmap)
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
    Vector3 deepColor{0.02F, 0.12F, 0.28F};
    float absorption = 0.35F;
    float foamStrength = 0.85F;
    float detailNormalStrength = 0.34F;
    float shorelineFoamStrength = 0.90F;
    float shorelineFoamMaxDepth = 2.0F;
    WaterScreenSpaceReflectionSettings ssrSettings{};
    float waveTimeSeconds = 0.0F;
    float waveTimeScale = 1.0F;
    float waveAmplitudeScale = 1.0F;
    float waveSpeedScale = 1.0F;
    bool surfaceDirty = true;
    Optional<Vector3> clipmapCameraOverride{};
    Optional<Vector2> swellTravelDirectionOverride{};

    [[nodiscard]] bool TryResolveClipmapCameraWorld(const GameWorld& world, Vector3& outWorld) const noexcept;
};

}  // namespace Spark
