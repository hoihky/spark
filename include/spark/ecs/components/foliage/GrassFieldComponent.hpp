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

    void SetEnabled(bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetTerrainObject(GameObject* object) noexcept { terrainProbe.BindTerrainObject(object); }
    [[nodiscard]] GameObject* GetTerrainObject() const noexcept { return terrainProbe.GetTerrainObject(); }

    void SetViewTargetObject(GameObject* object) noexcept { viewTargetObject = object; }
    [[nodiscard]] GameObject* GetViewTargetObject() const noexcept { return viewTargetObject; }

    /** Preferred streaming anchor (fly camera world position); overrides view-target object when set. */
    void SetStreamingViewWorld(const Vector3& worldPosition) noexcept;

    /** Rebuilds visible instances for the current render camera (call from demo <c>Render</c> before submit). */
    void PrepareForRender(
            GameObject& owner,
            const Vector3& viewWorldPosition,
            std::uint32_t sceneMaxGrassInstancesPerFrame = 0U);

    void SetBladeMesh(const SharedPtr<Mesh>& mesh) noexcept { bladeMesh = mesh; }
    [[nodiscard]] const SharedPtr<Mesh>& GetBladeMesh() const noexcept { return bladeMesh; }

    void SetAlbedoTexture(const SharedPtr<Texture2D>& texture) noexcept { albedoTexture = texture; }
    [[nodiscard]] const SharedPtr<Texture2D>& GetAlbedoTexture() const noexcept { return albedoTexture; }

    GrassFieldBounds& GetBounds() noexcept { return bounds; }
    [[nodiscard]] const GrassFieldBounds& GetBounds() const noexcept { return bounds; }

    GrassChunkScatterSettings& GetScatterSettings() noexcept { return scatterSettings; }
    [[nodiscard]] const GrassChunkScatterSettings& GetScatterSettings() const noexcept { return scatterSettings; }

    void SetNeighborhoodRingRadius(int radiusChunks) noexcept { neighborhood.SetRingRadiusChunks(radiusChunks); }
    [[nodiscard]] int GetNeighborhoodRingRadius() const noexcept { return neighborhood.GetRingRadiusChunks(); }

    void SetAlphaCutoff(float value) noexcept { alphaCutoff = value; }
    [[nodiscard]] float GetAlphaCutoff() const noexcept { return alphaCutoff; }

    void SetWindBendScale(float scale) noexcept { windBendScale = scale; }
    [[nodiscard]] float GetWindBendScale() const noexcept { return windBendScale; }

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
