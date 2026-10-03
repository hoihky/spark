#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"
#include "spark/scene/foliage/GrassChunkNeighborhood.hpp"
#include "spark/scene/foliage/GrassChunkScatterBuilder.hpp"
#include "spark/scene/foliage/GrassFieldBounds.hpp"
#include "spark/scene/foliage/GrassFieldChunkCache.hpp"
#include "spark/scene/foliage/GrassFieldStreamingPolicy.hpp"
#include "spark/scene/foliage/GrassTerrainSurfaceProbe.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/**
 * Terrain-following grass field with camera-centered chunk streaming (F2-01…F2-03).
 * Instances are collected by <c>FoliageSceneCollector</c> alongside legacy instanced meshes.
 */
class GrassFieldComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GrassField;

    GrassFieldComponent() = default;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool value) noexcept { enabled = value; }

    SPARK_SCRIPT_BIND(get_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_terrain_object)
    void SetTerrainObject(GameObject* object) noexcept { terrainProbe.BindTerrainObject(object); }
    [[nodiscard]] GameObject* GetTerrainObject() const noexcept { return terrainProbe.GetTerrainObject(); }

    SPARK_SCRIPT_BIND(set_view_target_object)
    void SetViewTargetObject(GameObject* object) noexcept { viewTargetObject = object; }
    [[nodiscard]] GameObject* GetViewTargetObject() const noexcept { return viewTargetObject; }

    /** Preferred streaming anchor (fly camera world position); overrides view-target object when set. */
    SPARK_SCRIPT_BIND(set_streaming_view_world)
    void SetStreamingViewWorld(const Vector3& worldPosition) noexcept;

    /** Rebuilds visible instances for the current render camera (call from demo <c>Render</c> before submit). */
    void PrepareForRender(
            GameObject& owner,
            const Vector3& viewWorldPosition,
            std::uint32_t sceneMaxGrassInstancesPerFrame = 0U);

    SPARK_SCRIPT_BIND(prepare_for_render)
    void PrepareForRenderInterop(
            GameObject* owner,
            const Vector3& viewWorldPosition,
            std::uint32_t sceneMaxGrassInstancesPerFrame) noexcept;

    void SetBladeMesh(const SharedPtr<Mesh>& mesh) noexcept { bladeMesh = mesh; }
    [[nodiscard]] const SharedPtr<Mesh>& GetBladeMesh() const noexcept { return bladeMesh; }

    void SetAlbedoTexture(const SharedPtr<Texture2D>& texture) noexcept { albedoTexture = texture; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetAlbedoTexture() const noexcept { return albedoTexture; }

    GrassFieldBounds& GetBounds() noexcept { return bounds; }
    [[nodiscard]] const GrassFieldBounds& GetBounds() const noexcept { return bounds; }

    GrassChunkScatterSettings& GetScatterSettings() noexcept { return scatterSettings; }
    [[nodiscard]] const GrassChunkScatterSettings& GetScatterSettings() const noexcept { return scatterSettings; }

    SPARK_SCRIPT_BIND(set_neighborhood_ring_radius)
    void SetNeighborhoodRingRadius(int radiusChunks) noexcept { neighborhood.SetRingRadiusChunks(radiusChunks); }

    SPARK_SCRIPT_BIND(get_neighborhood_ring_radius)
    [[nodiscard]] int GetNeighborhoodRingRadius() const noexcept { return neighborhood.GetRingRadiusChunks(); }

    SPARK_SCRIPT_BIND(set_alpha_cutoff)
    void SetAlphaCutoff(float value) noexcept { alphaCutoff = value; }

    SPARK_SCRIPT_BIND(get_alpha_cutoff)
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }

    SPARK_SCRIPT_BIND(set_wind_bend_scale)
    void SetWindBendScale(float scale) noexcept { windBendScale = scale; }

    SPARK_SCRIPT_BIND(get_wind_bend_scale)
    [[nodiscard]] float GetWindBendScale() const noexcept { return windBendScale; }

    SPARK_SCRIPT_BIND(bounds_set_half_extents)
    void BoundsSetHalfExtents(float halfX, float halfZ) noexcept { bounds.SetHalfExtentsMeters(halfX, halfZ); }

    SPARK_SCRIPT_BIND(scatter_set_chunk_size_meters)
    void ScatterSetChunkSizeMeters(float meters) noexcept { scatterSettings.SetChunkSizeMeters(meters); }

    SPARK_SCRIPT_BIND(scatter_set_density_per_square_meter)
    void ScatterSetDensityPerSquareMeter(float density) noexcept
    {
        scatterSettings.SetDensityPerSquareMeter(density);
    }

    SPARK_SCRIPT_BIND(scatter_set_max_view_distance_meters)
    void ScatterSetMaxViewDistanceMeters(float meters) noexcept
    {
        scatterSettings.SetMaxViewDistanceMeters(meters);
    }

    SPARK_SCRIPT_BIND(scatter_set_max_visible_instances)
    void ScatterSetMaxVisibleInstances(std::uint32_t count) noexcept
    {
        scatterSettings.SetMaxVisibleInstances(count);
    }

    SPARK_SCRIPT_BIND(scatter_set_max_cached_instances_per_chunk)
    void ScatterSetMaxCachedInstancesPerChunk(std::uint32_t count) noexcept
    {
        scatterSettings.SetMaxCachedInstancesPerChunk(count);
    }

    SPARK_SCRIPT_BIND(scatter_set_max_slope_degrees)
    void ScatterSetMaxSlopeDegrees(float degrees) noexcept { scatterSettings.SetMaxSlopeDegrees(degrees); }

    SPARK_SCRIPT_BIND(scatter_set_placement_radius_meters)
    void ScatterSetPlacementRadiusMeters(float meters) noexcept
    {
        scatterSettings.SetPlacementRadiusMeters(meters);
    }

    SPARK_SCRIPT_BIND(scatter_set_distance_fade_outer_fraction)
    void ScatterSetDistanceFadeOuterFraction(float fraction) noexcept
    {
        scatterSettings.SetDistanceFadeOuterFraction(fraction);
    }

    SPARK_SCRIPT_BIND(scatter_set_albedo_tint)
    void ScatterSetAlbedoTint(const Vector3& rgb) noexcept { scatterSettings.SetAlbedoTint(rgb); }

    void AppendInstanceRecords(Array<FoliageInstanceRecord>& outInstances) const;

private:
    enum class StreamingUpdatePass { ScatterOnly, ScatterAndMerge };

    [[nodiscard]] Vector3 ResolveStreamingViewWorld() const noexcept;
    [[nodiscard]] bool NeedsMergeRebuild(const Vector3& viewPos, const GrassChunkCoordinate& centerChunk) const noexcept;
    /** Scatters any in-range active chunks not yet built; returns true if new data was generated. */
    [[nodiscard]] bool ScatterChunksInViewRange(GameObject& owner, const Vector3& viewPos);
    void RefreshActiveChunks(GameObject& owner, const Vector3& viewPos, StreamingUpdatePass pass);

    bool enabled = true;
    bool hasStreamingViewWorld = false;
    Vector3 streamingViewWorld{};
    GameObject* viewTargetObject = nullptr;
    SharedPtr<Mesh> bladeMesh{};
    SharedPtr<Texture2D> albedoTexture{};
    float alphaCutoff = 0.42F;
    float windBendScale = 0.26F;

    GrassFieldBounds bounds{};
    GrassChunkScatterSettings scatterSettings{};
    GrassChunkNeighborhood neighborhood{};
    GrassTerrainSurfaceProbe terrainProbe{};
    GrassChunkScatterBuilder scatterBuilder{};
    GrassFieldChunkCache chunkCache{};
    Array<FoliageInstanceRecord> mergedInstances{};
    Array<GrassChunkCoordinate> activeChunkCoords{};
    GrassFieldStreamingPolicy streamingPolicy{};
    GrassChunkCoordinate lastMergedCenterChunk{};
    Vector3 lastMergedViewPos{};
    bool hasMergedOnce = false;
    std::uint32_t sceneGrassInstanceBudget = 0U;
    Array<bool> mergePickScratch{};
};

}  // namespace Spark
