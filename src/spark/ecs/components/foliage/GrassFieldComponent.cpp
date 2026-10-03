#include "spark/ecs/components/foliage/GrassFieldComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/foliage/GrassFieldChunkDistanceSort.hpp"
#include "spark/scene/foliage/GrassFieldInstanceDistanceSort.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

namespace {

constexpr float kMergeViewMoveThresholdMetersSq = 12.0F;  // ~3.5 m before re-merge
constexpr float kChunkDiagonalFactor = 1.42F;

[[nodiscard]] std::uint32_t ResolveInstanceBudget(
        const GrassChunkScatterSettings& settings,
        const std::uint32_t sceneMaxGrassInstancesPerFrame) noexcept {
    const std::uint32_t globalCap = SceneRenderParams::MaxFoliageInstances - 256U;
    std::uint32_t budget = settings.GetMaxVisibleInstances();
    if (sceneMaxGrassInstancesPerFrame > 0U && sceneMaxGrassInstancesPerFrame < budget) {
        budget = sceneMaxGrassInstancesPerFrame;
    }
    return budget < globalCap ? budget : globalCap;
}

[[nodiscard]] float ChunkScatterCullDistanceSq(const GrassChunkScatterSettings& settings) noexcept {
    const float chunkSize = settings.GetChunkSizeMeters();
    const float chunkCullDist = settings.GetMaxViewDistanceMeters() + chunkSize * kChunkDiagonalFactor;
    return chunkCullDist * chunkCullDist;
}

[[nodiscard]] bool IsChunkWithinScatterRange(
        const GrassChunkCoordinate& coord,
        const Vector3& viewPos,
        float chunkSize,
        float cullDistSq) noexcept {
    const float centerDx = coord.WorldCenterX(chunkSize) - viewPos.x;
    const float centerDz = coord.WorldCenterZ(chunkSize) - viewPos.z;
    return centerDx * centerDx + centerDz * centerDz <= cullDistSq;
}

float HorizontalDistanceSq(const Vector3& a, const Vector3& b) noexcept {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

void RebuildMergedInstances(
        const Array<GrassChunkCoordinate>& activeChunkCoords,
        const GrassFieldChunkCache& chunkCache,
        const Vector3& viewPos,
        float maxViewSq,
        std::uint32_t instanceBudget,
        Array<bool>& pickScratch,
        Array<FoliageInstanceRecord>& mergedInstances) {
    mergedInstances.Clear();
    for (std::size_t i = 0; i < activeChunkCoords.GetSize(); ++i) {
        if (mergedInstances.GetSize() >= instanceBudget) {
            break;
        }
        const GrassFieldChunkCache::ChunkPayload* payload = chunkCache.Find(activeChunkCoords[i]);
        if (payload == nullptr) {
            continue;
        }
        const Array<FoliageInstanceRecord>& chunkInstances = payload->GetInstances();
        const std::uint32_t remainingBudget =
                instanceBudget - static_cast<std::uint32_t>(mergedInstances.GetSize());

        if (chunkInstances.GetSize() <= remainingBudget) {
            for (std::size_t j = 0; j < chunkInstances.GetSize(); ++j) {
                if (mergedInstances.GetSize() >= instanceBudget) {
                    break;
                }
                const Matrix4& model = chunkInstances[j].GetModelMatrix();
                const Vector3 worldPos = model.TransformPoint(Vector3::Zero);
                if (HorizontalDistanceSq(worldPos, viewPos) > maxViewSq) {
                    continue;
                }
                mergedInstances.PushBack(chunkInstances[j]);
            }
        } else {
            GrassFieldInstanceDistanceSort::AppendNearestWithinView(
                    chunkInstances, viewPos, maxViewSq, remainingBudget, mergedInstances, pickScratch);
        }
    }
}

}  // namespace

void GrassFieldComponent::SetStreamingViewWorld(const Vector3& worldPosition) noexcept {
    streamingViewWorld = worldPosition;
    hasStreamingViewWorld = true;
}

void GrassFieldComponent::PrepareForRender(
        GameObject& owner,
        const Vector3& viewWorldPosition,
        const std::uint32_t sceneMaxGrassInstancesPerFrame) {
    SetStreamingViewWorld(viewWorldPosition);
    sceneGrassInstanceBudget = sceneMaxGrassInstancesPerFrame;
    RefreshActiveChunks(owner, viewWorldPosition, StreamingUpdatePass::ScatterAndMerge);
}

void GrassFieldComponent::PrepareForRenderInterop(
        GameObject* owner,
        const Vector3& viewWorldPosition,
        const std::uint32_t sceneMaxGrassInstancesPerFrame) noexcept {
    if (owner == nullptr) {
        return;
    }
    PrepareForRender(*owner, viewWorldPosition, sceneMaxGrassInstancesPerFrame);
}

void GrassFieldComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& context) {
    (void)timing;
    (void)context;
    if (!enabled || terrainProbe.GetTerrainObject() == nullptr) {
        mergedInstances.Clear();
        hasMergedOnce = false;
        return;
    }
    if (viewTargetObject == nullptr && !hasStreamingViewWorld) {
        mergedInstances.Clear();
        hasMergedOnce = false;
        return;
    }
    RefreshActiveChunks(owner, ResolveStreamingViewWorld(), StreamingUpdatePass::ScatterOnly);
}

Vector3 GrassFieldComponent::ResolveStreamingViewWorld() const noexcept {
    if (hasStreamingViewWorld) {
        return streamingViewWorld;
    }
    if (viewTargetObject == nullptr) {
        return Vector3::Zero;
    }
    const TransformComponent* viewTransform = viewTargetObject->GetComponent<TransformComponent>();
    if (viewTransform == nullptr) {
        return Vector3::Zero;
    }
    return viewTargetObject->GetWorldMatrix().TransformPoint(viewTransform->GetLocalTransform().translation);
}

bool GrassFieldComponent::NeedsMergeRebuild(
        const Vector3& viewPos,
        const GrassChunkCoordinate& centerChunk) const noexcept {
    if (!hasMergedOnce) {
        return true;
    }
    if (!(centerChunk == lastMergedCenterChunk)) {
        return true;
    }
    return HorizontalDistanceSq(viewPos, lastMergedViewPos) > kMergeViewMoveThresholdMetersSq;
}

bool GrassFieldComponent::ScatterChunksInViewRange(GameObject& owner, const Vector3& viewPos) {
    if (activeChunkCoords.IsEmpty()) {
        return false;
    }

    const float chunkSize = scatterSettings.GetChunkSizeMeters();
    const float cullDistSq = ChunkScatterCullDistanceSq(scatterSettings);
    const Vector3 fieldOrigin = owner.GetWorldMatrix().TransformPoint(Vector3::Zero);
    bool scatteredAny = false;

    for (std::size_t i = 0; i < activeChunkCoords.GetSize(); ++i) {
        const GrassChunkCoordinate& coord = activeChunkCoords[i];
        if (!IsChunkWithinScatterRange(coord, viewPos, chunkSize, cullDistSq)) {
            continue;
        }

        GrassFieldChunkCache::ChunkPayload& payload = chunkCache.FindOrInsert(coord);
        if (payload.HasScatterForChunk()) {
            continue;
        }

        payload.EditInstances().Clear();
        scatterBuilder.ScatterChunk(
                coord,
                scatterSettings,
                bounds,
                fieldOrigin,
                terrainProbe,
                payload.EditInstances());
        payload.MarkScatterComplete();
        scatteredAny = true;
    }

    return scatteredAny;
}

void GrassFieldComponent::RefreshActiveChunks(
        GameObject& owner,
        const Vector3& viewPos,
        const StreamingUpdatePass pass) {
    const float chunkSize = scatterSettings.GetChunkSizeMeters();
    const GrassChunkCoordinate center = GrassChunkCoordinate::FromWorldPosition(viewPos.x, viewPos.z, chunkSize);

    if (activeChunkCoords.IsEmpty() || streamingPolicy.NeedsChunkScatterRefresh(center)) {
        neighborhood.BuildAround(center, activeChunkCoords);
        chunkCache.RemoveExcept(activeChunkCoords);
        streamingPolicy.NotifyChunkScatterRefreshed(center);
        hasMergedOnce = false;
    }

    if (ScatterChunksInViewRange(owner, viewPos)) {
        hasMergedOnce = false;
    }

    if (pass == StreamingUpdatePass::ScatterOnly) {
        return;
    }

    if (!NeedsMergeRebuild(viewPos, center)) {
        return;
    }

    GrassFieldChunkDistanceSort::SortByViewDistance(activeChunkCoords, viewPos.x, viewPos.z, chunkSize);

    const std::uint32_t sceneBudget = sceneGrassInstanceBudget > 0U
            ? sceneGrassInstanceBudget
            : SceneRenderParams::DefaultMaxGrassInstancesPerFrame;
    const std::uint32_t instanceBudget = ResolveInstanceBudget(scatterSettings, sceneBudget);
    const float maxViewSq = scatterSettings.GetMaxViewDistanceMeters() *
            scatterSettings.GetMaxViewDistanceMeters();
    RebuildMergedInstances(
            activeChunkCoords,
            chunkCache,
            viewPos,
            maxViewSq,
            instanceBudget,
            mergePickScratch,
            mergedInstances);
    lastMergedViewPos = viewPos;
    lastMergedCenterChunk = center;
    hasMergedOnce = true;
}

void GrassFieldComponent::AppendInstanceRecords(Array<FoliageInstanceRecord>& outInstances) const {
    if (!enabled || mergedInstances.IsEmpty()) {
        return;
    }
    for (std::size_t i = 0; i < mergedInstances.GetSize(); ++i) {
        if (outInstances.GetSize() >= SceneRenderParams::MaxFoliageInstances) {
            break;
        }
        outInstances.PushBack(mergedInstances[i]);
    }
}

}  // namespace Spark
