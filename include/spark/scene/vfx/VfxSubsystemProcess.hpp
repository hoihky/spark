#pragma once

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark {

/** Drains <c>VfxSubsystem</c> queues after component <c>OnUpdate</c> runs. */
inline void ProcessVfx(GameWorld& world) {
    world.GetVfxSubsystem().Process(world);
}

}  // namespace Spark
