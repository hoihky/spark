#pragma once

#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace Spark {

/** Evaluates walkability / path-blocking from tile metadata and walk rules. */
class TilemapGameplayRuleEvaluator final {
public:
    [[nodiscard]] bool BlocksGameplayPath(
            const TileCell& cell,
            const TileDefinition& definition,
            TilemapGameplayWalkRule rule) const noexcept;
};

}  // namespace Spark
