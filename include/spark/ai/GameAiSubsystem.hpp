#pragma once

#include "spark/ai/AiSubsystem.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
struct FrameTiming;

/** @deprecated Prefer <c>AiSubsystem::RunFrame</c>. */
inline void SimulateGameAi(GameWorld& world, const FrameTiming& timing, IEngineContext& context) {
    AiSubsystem::RunFrame(world, timing, context);
}

}  // namespace Spark
