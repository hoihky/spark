#include "spark/scene/tilemap/TilemapGameplayRules.hpp"

namespace Spark {

bool TilemapGameplayRuleEvaluator::BlocksGameplayPath(
        const TileCell& cell,
        const TileDefinition& definition,
        const TilemapGameplayWalkRule rule) const noexcept {
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
