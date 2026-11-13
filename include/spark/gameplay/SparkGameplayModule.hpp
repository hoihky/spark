#pragma once

namespace Spark {

class ComponentSnapshotRegistry;
class GameWorld;

/** Registers gameplay/game-flow snapshot handlers (call once at startup). */
void RegisterSparkGameplaySnapshotHandlers(ComponentSnapshotRegistry& registry);

}  // namespace Spark
