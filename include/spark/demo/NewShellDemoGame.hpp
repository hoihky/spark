#pragma once

#include "spark/engine/IGame.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

/** Same interactive launcher + demos as the SparkDemo executable (3D, GUI, Sky, Particles). */
[[nodiscard]] UniquePtr<IGame> NewShellDemoGame();

}  // namespace Spark
