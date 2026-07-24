#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"

namespace Spark {

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

    BakeTilemapGameplayGrid(*tilemap, walkRule, grid);
    frame = MakeTilemapGridFrame(
            owner.GetWorldMatrix(),
            tilemap->GetTileWorldSize(),
            tilemap->GetMapWidth(),
            tilemap->GetMapHeight());
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
