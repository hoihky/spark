#include "spark/gameplay/SparkGameplayModule.hpp"

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"

namespace Spark {

void RegisterSparkGameplaySnapshotHandlers(ComponentSnapshotRegistry& /*registry*/) {
    // Gameplay and game-flow component snapshots register from ComponentSnapshotRegistry::RegisterBuiltInHandlers.
}

}  // namespace Spark
