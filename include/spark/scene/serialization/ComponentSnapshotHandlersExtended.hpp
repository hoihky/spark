#pragma once

namespace Spark {

class ComponentSnapshotRegistry;

/** Registers directional_light, spot_light, sky, sprite, spatial_policy handlers on the registry. */
void RegisterExtendedSnapshotHandlers(ComponentSnapshotRegistry& registry);

}  // namespace Spark
