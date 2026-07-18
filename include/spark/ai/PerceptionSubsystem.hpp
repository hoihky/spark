#pragma once

namespace Spark {

class GameWorld;

/** Updates every enabled <c>PerceptionSensorComponent</c> with nearby targets. */
void ProcessPerceptionSensors(GameWorld& world) noexcept;

}  // namespace Spark
