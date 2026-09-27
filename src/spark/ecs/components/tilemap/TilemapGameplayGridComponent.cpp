#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"

namespace Spark {

namespace {

void SyncGridFrame(TilemapGridFrame& outFrame, const GameObject& owner, const TilemapComponent& tilemap) noexcept {
    outFrame = TilemapGridFrame::FromMapTransform(
            owner.GetWorldMatrix(),
            tilemap.GetTileWorldSize(),
            tilemap.GetMapWidth(),
            tilemap.GetMapHeight());
}

}  // namespace

void TilemapGameplayGridComponent::RebakeRegion(
        const GameObject& owner,
        const TilemapCellRegion& region) noexcept {
    const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr || tilemap->GetMapWidth() == 0U || tilemap->GetMapHeight() == 0U || region.IsEmpty()) {
        return;
    }
    TilemapGameplayGridBaker{}.BakeRegion(*tilemap, walkRule, grid, region);
    SyncGridFrame(frame, owner, *tilemap);
    rebakeRequested = false;
}

void TilemapGameplayGridComponent::RebakeIfNeeded(const GameObject& owner) noexcept {
    if (!autoRebake && !rebakeRequested) {
        return;
    }
    const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr || tilemap->GetMapWidth() == 0 || tilemap->GetMapHeight() == 0) {
        grid.Resize(0, 0);
        rebakeRequested = false;
        return;
    }

    TilemapGameplayGridBaker{}.BakeFull(*tilemap, walkRule, grid);
    SyncGridFrame(frame, owner, *tilemap);
    rebakeRequested = false;
}

void TilemapGameplayGridComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& context) {
    (void)timing;
    (void)context;
    RebakeIfNeeded(owner);
}

}  // namespace Spark
