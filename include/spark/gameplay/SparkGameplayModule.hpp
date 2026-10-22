#pragma once

namespace Spark {

class ComponentSnapshotRegistry;
class GameWorld;

/** Registers gameplay/game-flow snapshot handlers (call once at startup). */
void RegisterSparkGameplaySnapshotHandlers(ComponentSnapshotRegistry& registry);

/** Post-2D-physics hooks (pickup deferred destroys, etc.). */
void SparkGameplayAfterPhysics2D(GameWorld& world);

}  // namespace Spark
