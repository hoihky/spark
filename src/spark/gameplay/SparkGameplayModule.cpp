#include "spark/gameplay/SparkGameplayModule.hpp"

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/ComponentSnapshotHandlersGameFlow.hpp"
#include "spark/scene/serialization/ComponentSnapshotHandlersGameplay.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"

namespace Spark {

void RegisterSparkGameplaySnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterGameFlowSnapshotHandlers(registry);
    RegisterGameplaySnapshotHandlers(registry);
}

namespace {

struct SparkGameplayAutoRegister {
    SparkGameplayAutoRegister() {
        RegisterSparkGameplaySnapshotHandlers(ComponentSnapshotRegistry::Default());
    }
};

const SparkGameplayAutoRegister g_sparkGameplayAutoRegister{};

}  // namespace

}  // namespace Spark
