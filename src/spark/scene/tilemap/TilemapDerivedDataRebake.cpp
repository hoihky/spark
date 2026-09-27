#include "spark/scene/tilemap/TilemapDerivedDataRebake.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

TilemapDerivedDataRebaker::Result TilemapDerivedDataRebaker::RebakeForRevision(
        GameObject& owner,
        const TilemapEditRevision& revision) const noexcept {
    return RebakeForRevision(owner, revision, Options{}, nullptr, nullptr);
}

TilemapDerivedDataRebaker::Result TilemapDerivedDataRebaker::RebakeForRevision(
        GameObject& owner,
        const TilemapEditRevision& revision,
        const Options& options,
        GameWorld* worldForPhysics,
        PhysicsSubsystem* physics) const noexcept {
    Result result{};
    TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr || tilemap->GetMapWidth() == 0U || tilemap->GetMapHeight() == 0U) {
        return result;
    }

    const TilemapCellRegion region = TilemapCellRegion::FromRevision(
            revision, tilemap->GetMapWidth(), tilemap->GetMapHeight(), options.neighborMargin);
    if (region.IsEmpty()) {
        return result;
    }
    result.rebakedRegion = region;

    if (options.gameplayGrid) {
        if (TilemapGameplayGridComponent* grid = owner.GetComponent<TilemapGameplayGridComponent>()) {
            grid->RebakeRegion(owner, region);
            result.gameplayGridRebaked = true;
        }
    }

    if (options.autotile) {
        if (TilemapAutotileComponent* autotile = owner.GetComponent<TilemapAutotileComponent>()) {
            autotile->RebuildRegion(owner, region);
            result.autotileRebaked = true;
        }
    }

    if (options.physicsQueryStatics && worldForPhysics != nullptr && physics != nullptr) {
        physics->GetQueries2D().RebuildStatics(*worldForPhysics);
        result.physicsQueryStaticsRebuilt = true;
    }

    return result;
}

}  // namespace Spark
