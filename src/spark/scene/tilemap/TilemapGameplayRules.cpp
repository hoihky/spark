#include "spark/scene/tilemap/TilemapGameplayRules.hpp"

namespace Spark {

bool TileBlocksGameplayPath(
        const TileCell& cell,
        const TileDefinition& definition,
        const TilemapGameplayWalkRule rule) noexcept {
    if (cell.IsEmpty()) {
        return true;
    }
    if (definition.HasFlag(TileDefinitionFlags::ForceWalkable)) {
        return false;
    }
    if (definition.HasFlag(TileDefinitionFlags::BlocksPathfinding)) {
        return true;
    }
    switch (rule) {
        case TilemapGameplayWalkRule::OccupiedWalkable:
            return false;
        case TilemapGameplayWalkRule::CollisionAligned:
            return definition.ContributesCollision();
        case TilemapGameplayWalkRule::DefinitionAndFlags:
            return definition.ContributesCollision();
    }
    return definition.ContributesCollision();
}

}  // namespace Spark
