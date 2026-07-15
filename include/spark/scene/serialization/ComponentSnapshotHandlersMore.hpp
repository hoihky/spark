#pragma once

namespace Spark {

class ComponentSnapshotRegistry;

/** Registers text_overlay, particle_emitter, terrain, and 3D physics snapshot handlers. */
void RegisterMoreSnapshotHandlers(ComponentSnapshotRegistry& registry);

}  // namespace Spark
